package;

import Types.TileMap;
import Types.TileSet;
import Types.Weapon;
import flixel.FlxG;
import flixel.FlxState;
import flixel.util.FlxColor;

enum STATE
{
	CONNECT;
	STATIC_DATA;
	WORLD_STATE;
	PLAY;
}

class PlayState extends FlxState
{
	var state:STATE = CONNECT;

	var udp_client:UDPClient;
	var tcp_client:TCPClient;

	var last_sent_ask_map:Float = -10;
	var ask_map_timeout:Int = 5;
	var ask_map_attempts:Int = 0;

	var last_sent_ask_world:Float = -10;
	var ask_world_timeout:Int = 5;
	var ask_world_attempts:Int = 0;

	var tilemap:TileMap;
	var tileset:TileSet;
	var weapons:haxe.ds.Vector<Weapon>;

	var started:Bool = false;
	var next_deadline:Float = 0;

	var name:String;

	var world:World;

	override public function new(name:String)
	{
		super();
		this.name = name;
	}

	override public function create()
	{
		super.create();

		persistentUpdate = true;
		persistentDraw = true;

		// network
		udp_client = new UDPClient(Config.server_ip, Config.server_port);
		tcp_client = new TCPClient();

		openSubState(new LoadingState());
	}

	override public function onFocusLost()
	{
		trace("onFocusLost");
	}

	override public function onFocus()
	{
		trace("onFocus");
	}

	override public function update(elapsed:Float)
	{
		super.update(elapsed);
		udp_client.update();

		switch (state)
		{
			case CONNECT:
				startConnection();
			case STATIC_DATA:
				getStatic();
			case WORLD_STATE:
				getWorldState();
			case PLAY:
				gameloop();
		}
	}

	function gameloop()
	{
		while (udp_client.hasMessages())
		{
			var msg = udp_client.popMessage();
			switch (msg.opcode)
			{
				case Config.OP_SNAPSHOT:
					var snapshot = Utils.readSnapshot(msg.content);
					world.handleSnapshot(snapshot);
				default:
					trace("Got unexpected message while waiting for SNAPSHOT: " + Utils.stringOfNetworkFrame(msg) + ", (dropped)");
			}
		}

		if (Utils.Time.nowInMilliseconds() > next_deadline)
		{
			var frame = world.makeControlFrame();
			var frame_bytes = Utils.bytesOfControlFrame(frame);

			udp_client.write({
				opcode: Config.OP_CONTROL_FRAME,
				size: frame_bytes.length,
				content: frame_bytes,
			});

			next_deadline = Utils.Time.nextClientDeadline();
		}
	}

	function getWorldState()
	{
		if (ask_world_attempts >= 10)
			FlxG.switchState(new WelcomeState());

		if (Sys.time() > last_sent_ask_world + ask_world_timeout)
		{
			trace("Asking for world configuration..." + (ask_world_attempts > 0 ? ' ($ask_world_attempts)' : ""));

			udp_client.write({
				opcode: Config.OP_CONFIG,
				size: name.length,
				content: haxe.io.Bytes.ofString(name),
			});

			last_sent_ask_world = Sys.time();
			ask_world_attempts++;
		}

		while (udp_client.hasMessages())
		{
			var msg = udp_client.popMessage();
			switch (msg.opcode)
			{
				case Config.OP_CONFIG:
					var world_config = Utils.readConfig(msg.content);

					Config.ACK_SIZE = world_config.ack_size;
					Config.CLIENT_RATE = world_config.client_rate;
					Config.SERVER_RATE = world_config.server_rate;
					Utils.Time.startNow(world_config.snapshot.server_tick);
					next_deadline = Utils.Time.nextClientDeadline();

					world = new World(world_config, tilemap, tileset, weapons);
					add(world);

					udp_client.write({
						opcode: Config.OP_CLIENT_READY,
						size: 0,
						content: haxe.io.Bytes.alloc(0),
					});

					started = true;
					trace("Started");

					state = PLAY;
					break;
					#if debug
					default:
						trace("[" + Utils.Time.nowInMilliseconds() + " ms] Got unexpected message while waiting for CONFIG: "
							+ Utils.stringOfNetworkFrame(msg) + ", (dropped)");
					#end
			}
		}
	}

	function getStatic()
	{
		if (ask_map_attempts >= 10)
			FlxG.switchState(new WelcomeState());

		if (!tcp_client.connected && Sys.time() > last_sent_ask_map + ask_map_timeout)
		{
			trace("Asking for static data..." + (ask_map_attempts > 0 ? ' ($ask_map_attempts)' : ""));

			udp_client.write({
				opcode: Config.OP_STATIC_INFO,
				size: 0,
				content: haxe.io.Bytes.alloc(0),
			});

			last_sent_ask_map = Sys.time();
			ask_map_attempts++;
		}

		if (!tcp_client.connected)
			while (udp_client.hasMessages())
			{
				var message = udp_client.popMessage();
				switch (message.opcode)
				{
					case Config.OP_STATIC_INFO:
						var reader = new ByteReader(message.content);

						var n_files = reader.readInt32();
						var port = reader.readInt32();

						if (subState != null)
						{
							cast(subState, LoadingState).min_progress = 0;
							cast(subState, LoadingState).max_progress = 100;
						}

						tcp_client.download(Config.server_ip, port, n_files);
						break;
					default:
						trace("got opcode " + Utils.stringOfOpcode(message.opcode) + " instead of OP_STATIC_INFO");
				}
			}
		else
		{
			tcp_client.update();
			var cur_file_progress = tcp_client.total_size <= 0 ? 0 : tcp_client.read_so_far / tcp_client.total_size;
			var progress = Math.floor((tcp_client.cur_file + cur_file_progress) / tcp_client.n_files * 100);

			if (subState != null)
				cast(subState, LoadingState).progress = progress;

			if (tcp_client.done)
			{
				closeSubState();
				tilemap = Utils.readTileMap(tcp_client.files[0]);
				tileset = Utils.readTileSet(tcp_client.files[1]);

				// reconstruct collision map
				for (i in 0...tilemap.tiles.length)
					if (tilemap.tiles[i] == 0)
						tilemap.collisions.push(false);
					else
						tilemap.collisions.push(tileset.solid[tilemap.tiles[i] - 1]);

				weapons = Utils.readWeapons(tcp_client.files[2]);

				state = WORLD_STATE;
			}
		}
	}

	function startConnection()
	{
		if (udp_client.connection_attempts >= 10)
			FlxG.switchState(new WelcomeState());

		udp_client.doInit();
		if (udp_client.initialized)
			state = STATIC_DATA;
	}
}
