import Types.Control;
import Types.ControlFrame;
import Types.EntityDesc;
import Types.Movement;
import Types.NetworkFrame;
import Types.Snapshot;
import Types.Timed;
import Types.Weapon;

class Utils
{
	public static function byteToBinaryString(byte:Int)
	{
		var str = '';
		var aux = byte;

		for (i in 0...8)
		{
			str = (aux % 2 == 0 ? '0' : '1') + str;
			aux >>= 1;
		}
		return str;
	}

	public static function bytesToBinaryString(bytes:haxe.io.Bytes)
	{
		var str = '';
		for (i in 0...bytes.length)
			str = byteToBinaryString(bytes.get(i)) + str;
		return str;
	}

	public static function lerp(t0:Float, v0:Float, t1:Float, v1:Float, t:Float):Float
	{
		return (t1 - t) / (t1 - t0) * v0 + (t - t0) / (t1 - t0) * v1;
	}

	public static function lerpInt(t0:Float, v0:Int, t1:Float, v1:Int, t:Float):Float
	{
		return (t1 - t) / (t1 - t0) * v0 + (t - t0) / (t1 - t0) * v1;
	}

	public static function lerpEntityDesc(v0:Timed<EntityDesc>, v1:Timed<EntityDesc>, tick:Float):Timed<EntityDesc>
	{
		return {
			tick: tick,
			val: {
				id: v0.val.id,
				type: v0.val.type,
				name: v0.val.name,
				health: lerp(v0.tick, v0.val.health, v1.tick, v1.val.health, tick),
				max_health: lerp(v0.tick, v0.val.max_health, v1.tick, v1.val.max_health, tick),
				x: lerp(v0.tick, v0.val.x, v1.tick, v1.val.x, tick),
				y: lerp(v0.tick, v0.val.y, v1.tick, v1.val.y, tick),
				radius: lerp(v0.tick, v0.val.radius, v1.tick, v1.val.radius, tick),
				move_speed: lerp(v0.tick, v0.val.move_speed, v1.tick, v1.val.move_speed, tick),
				weapons: v0.val.weapons,
				weapon_i: v0.val.weapon_i,
				random_state: v0.val.random_state,
			}
		}
	}

	public static function interpolateRingTimedDesc(ring:Ring<Timed<EntityDesc>>, tick:Float):Timed<EntityDesc>
	{
		if (ring.count() < 2)
			return null;

		var i_low = -1;
		var i_high = -1;
		for (i in 0...ring.count())
		{
			var tick_i = ring.get(i).tick;
			if (tick_i > tick)
				i_high = i;
			else if (tick_i <= tick)
			{
				i_low = i;
				// ticks are ordered in Ring so we can stop now
				break;
			}
		}

		if (i_low == -1)
		{
			// #if debug
			// trace('ERROR interpolate $tick: couldn\'t find a low value');
			// #end
			return null;
		}

		var v_low = ring.get(i_low);

		if (i_high == -1)
		{
			// #if debug
			// trace('ERROR interpolate $tick: couldn\'t find a high value, returning low value');
			// #end
			return v_low;
		}

		var v_high = ring.get(i_high);

		return lerpEntityDesc(v_low, v_high, tick);
	}

	public static function stringOfOpcode(opcode:Int)
	{
		switch (opcode)
		{
			case Config.OP_PING:
				return "OP_PING";
			case Config.OP_PONG:
				return "OP_PONG";
			case Config.OP_BINARY:
				return "OP_BINARY";
			case Config.OP_INIT:
				return "INIT";
			case Config.OP_CONFIG:
				return "CONFIG";
			case Config.OP_STATIC_INFO:
				return "STATIC_INFO";
			case Config.OP_CLIENT_READY:
				return "OP_CLIENT_READY";
			case Config.OP_SNAPSHOT:
				return "SNAPSHOT";
			case Config.OP_CONTROL_FRAME:
				return "CONTROL_FRAME";
			default:
				return "" + opcode;
		}
	}

	public static function stringOfNetworkFrame(frame:NetworkFrame)
	{
		var opcode = stringOfOpcode(frame.opcode);
		var size = frame.size;
		return '[NetworkFrame opcode:$opcode size:$size]';
	}

	public static function stringOfSnapshot(snapshot:Snapshot)
	{
		var id = snapshot.id;
		var server_tick = snapshot.server_tick;
		var client_tick = snapshot.client_tick;
		var n = snapshot.entities.length;
		return '[Snapshot $id at tick s$server_tick c$client_tick, entities:$n]';
	}

	public static function stringOfMovements(move:Array<Movement>)
	{
		var up = "0";
		var down = "0";
		var left = "0";
		var right = "0";

		for (m in move)
		{
			switch (m)
			{
				case UP:
					up = "1";
				case DOWN:
					down = "1";
				case LEFT:
					left = "1";
				case RIGHT:
					right = "1";
			}
		}

		return up + down + left + right;
	}

	public static function stringOfControl(control:Control)
	{
		var movement = stringOfMovements(control.movement);
		var shoot = control.shoot ? "1" : "0";
		var angle = control.facing_angle * 180 / Math.PI;
		return '[Control move:$movement shoot:$shoot angle:$angle]';
	}

	public static function stringOfControlFrame(frame:ControlFrame)
	{
		var tick = frame.client_tick;
		var last_snapshot = frame.last_snapshot;
		var control = stringOfControl(frame.control);
		var ack = bytesToBinaryString(frame.ack);
		return '[ControlFrame at s$last_snapshot c$tick, control:$control, ack:$ack]';
	}

	public static function readEntity(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0, player:Bool = false)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readEntity: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readEntity(reader, player);
	}

	static function _readEntity(reader:ByteReader, player:Bool = false)
	{
		var ID = reader.readInt32();

		if (ID == -1)
		{
			// player is probably dead
			return null;
		}

		var type = -1;
		if (!player)
		{
			type = reader.readInt8();
		}

		var name = reader.readString();
		var health = reader.readFloat();
		var max_health = reader.readFloat();
		var x = reader.readFloat();
		var y = reader.readFloat();
		var radius = reader.readFloat();

		var move_speed = -1.0;
		if (player)
		{
			move_speed = reader.readFloat();
		}

		var weapons = new haxe.ds.Vector<Int>(Config.MAX_N_WEAPONS);
		for (i in 0...Config.MAX_N_WEAPONS)
		{
			var id = reader.readInt32();
			if (id < 0)
				weapons[i] = -1;
			else
				weapons[i] = id;
		}

		var weapon_i = reader.readInt32();

		var random_state = reader.readBytes(Config.XORSHIFT64PLUS_STATE_SIZE);

		var res = {
			id: ID,
			type: type,
			name: name,
			health: health,
			max_health: max_health,
			x: x,
			y: y,
			radius: radius,
			move_speed: move_speed,
			weapons: weapons,
			weapon_i: weapon_i,
			random_state: random_state,
		};

		return res;
	}

	public static function readSnapshot(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readSnapshot: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readSnapshot(reader);
	}

	static function _readSnapshot(reader:ByteReader)
	{
		var id = reader.readInt32();
		var server_tick = reader.readInt32();
		var client_tick = reader.readInt32();

		var player = readEntity(reader, true);

		var n_entities = reader.readInt32();
		var entities = new haxe.ds.Vector<EntityDesc>(n_entities);
		for (i in 0...n_entities)
			entities[i] = readEntity(reader, false);

		var n_despawned = reader.readInt32();
		var despawned = new haxe.ds.Vector<Int>(n_despawned);
		for (i in 0...n_despawned)
			despawned[i] = reader.readInt32();

		var res = {
			id: id,
			server_tick: server_tick,
			client_tick: client_tick,
			player: player,
			entities: entities,
			despawned: despawned,
		};

		return res;
	}

	public static function readTileMap(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readTileMap: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readTileMap(reader);
	}

	public static function _readTileMap(reader:ByteReader)
	{
		var width = reader.readInt32();
		var height = reader.readInt32();
		var tile_size = reader.readInt8();
		var scale = reader.readInt8();

		var tiles = new Array<Int>();
		var flip = new Array<Int>();
		for (i in 0...width * height)
		{
			var id:UInt = reader.readInt32();
			var flip_hori = ((id >> 31) % 2 == 1) ? 4 : 0;
			var flip_vert = ((id >> 30) % 2 == 1) ? 2 : 0;
			var flip_diag = ((id >> 29) % 2 == 1) ? 1 : 0;
			flip.push(flip_vert + flip_hori + flip_diag);
			tiles.push((id << 3) >> 3);
		}

		return {
			width: width,
			height: height,
			tile_size: tile_size,
			scale: scale,
			tiles: tiles,
			flip: flip,
			collisions: [],
		}
	}

	public static function readTileSet(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readTileSet: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readTileSet(reader);
	}

	public static function _readTileSet(reader:ByteReader)
	{
		var width = reader.readInt32();
		var height = reader.readInt32();
		var tile_size = reader.readInt8();

		var solid_bytes = reader.readBytes(Math.ceil(width * height / 8));
		var solid = [];
		for (i in 0...solid_bytes.length)
		{
			var byte = solid_bytes.get(i);
			var solid_ = [];
			for (i in 0...8)
			{
				solid_.push(byte % 2 == 1);
				byte >>= 1;
			}
			solid = solid.concat(solid_);
		}

		// this should never be big enough to reach Int64 realm
		var data_len = reader.readInt64();
		var data = reader.readBytes(data_len.low);

		return {
			width: width,
			height: height,
			tile_size: tile_size,
			solid: solid,
			data: flash.display.BitmapData.fromBytes(openfl.utils.ByteArray.fromBytes(data)),
		}
	}

	public static function readWeapons(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readWeapons: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readWeapons(reader);
	}

	public static function _readWeapons(reader:ByteReader)
	{
		var n = reader.readInt32();

		var weapons = new haxe.ds.Vector<Weapon>(n);
		for (i in 0...n)
			weapons[i] = readWeapon(reader);

		return weapons;
	}

	public static function readWeapon(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readWeapon: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readWeapon(reader);
	}

	public static function _readWeapon(reader:ByteReader)
	{
		var id = reader.readInt32();
		var name = reader.readString();
		var spread = reader.readFloat();
		var rate = reader.readFloat();
		var damage = reader.readFloat();
		var range = reader.readInt32();
		var bullet_speed = reader.readInt32();
		var bullet_count = reader.readInt32();

		return {
			id: id,
			name: name,
			spread: spread,
			rate: rate,
			damage: damage,
			range: range,
			bullet_speed: bullet_speed,
			bullet_count: bullet_count,
		};
	}

	public static function readConfig(?bytes:haxe.io.Bytes, ?reader:ByteReader, from:Int = 0)
	{
		if (reader == null)
		{
			if (bytes == null)
				trace("ERROR readConfig: please provide either bytes or reader");
			reader = new ByteReader(bytes);
			reader.seek(from);
		}
		return _readConfig(reader);
	}

	public static function _readConfig(reader:ByteReader)
	{
		var client_rate = reader.readInt8();
		var server_rate = reader.readInt8();
		var ack_size = reader.readInt8();
		var snapshot = readSnapshot(reader);

		var res = {
			client_rate: client_rate,
			server_rate: server_rate,
			ack_size: ack_size,
			snapshot: snapshot,
		};

		return res;
	}

	// Encoding:
	// bits 0-3: movement (summed)
	//      - 0x0000 : NONE
	//      - 0x1000 : UP
	//      - 0x0100 : DOWN
	//      - 0x0010 : LEFT
	//      - 0x0001 : RIGHT
	// bit 7: shoot?
	// bits 8-40: facing_angle
	public static function bytesOfControl(control:Control)
	{
		var movement_byte = 0;
		for (move in control.movement)
			switch (move)
			{
				case UP:
					movement_byte += 8;
				case DOWN:
					movement_byte += 4;
				case LEFT:
					movement_byte += 2;
				case RIGHT:
					movement_byte += 1;
			}

		var weapon = control.change_weapon ? control.new_weapon_i + 1 : 0;
		var first_byte = (movement_byte << 4) + (weapon << 2) + (control.run ? 2 : 0) + (control.shoot ? 1 : 0);

		var bytes = haxe.io.Bytes.alloc(5);
		bytes.set(0, first_byte);
		bytes.setFloat(1, control.facing_angle);

		return bytes;
	}

	static public function bytesOfNetworkFrame(frame:NetworkFrame)
	{
		var b = haxe.io.Bytes.alloc(frame.size + 5);
		b.set(0, frame.opcode);
		b.setInt32(1, frame.size);
		b.blit(5, frame.content, 0, frame.size);
		return b;
	}

	static public function bytesOfControlFrame(frame:ControlFrame)
	{
		var control_bytes = bytesOfControl(frame.control);

		var bytes = haxe.io.Bytes.alloc(8 + frame.ack.length + control_bytes.length);

		bytes.setInt32(0, frame.client_tick);
		bytes.setInt32(4, frame.last_snapshot);
		bytes.blit(8, frame.ack, 0, frame.ack.length);
		bytes.blit(8 + frame.ack.length, control_bytes, 0, control_bytes.length);

		return bytes;
	}
}

class Time
{
	public static var start_time:Float = -1;

	public static function initialized()
		return start_time > 0;

	public static function startNow(tick:Float)
		#if sys
		start_time = Sys.time() * 1000.0 - tick / Config.CLIENT_RATE * 1000.0;
		#else
		start_time = Date.now().getTime() - tick / Config.CLIENT_RATE * 1000.0;
		#end

	public static function now():Float
	{
		#if debug
		if (!initialized())
			trace("now: not initialized yet");
		#end
		#if sys
		return Sys.time() * 1000.0 - start_time;
		#else
		return Date.now().getTime() - start_time;
		#end
	}

	public static function nowInMilliseconds():Float
		return now();

	public static function msToClientTicks(ms:Float):Float
	{
		var client_period = 1.0 / Config.CLIENT_RATE;
		return Math.floor(ms / (client_period * 1000));
	}

	public static function msToServerTicks(ms:Float):Float
	{
		var server_period = 1.0 / Config.SERVER_RATE;
		return Math.floor(ms / (server_period * 1000));
	}

	public static function nowInClientTicks():Float
		return msToClientTicks(nowInMilliseconds());

	public static function nowInServerTicks():Float
		return msToServerTicks(nowInMilliseconds());

	public static function nextClientDeadline()
	{
		var client_period = 1.0 / Config.CLIENT_RATE;
		var now = nowInMilliseconds();
		var n_periods = Math.ceil(now / (client_period * 1000));
		return n_periods * client_period * 1000;
	}

	public static function nextServerDeadline()
	{
		var server_period = 1.0 / Config.SERVER_RATE;
		var now = nowInMilliseconds();
		var n_periods = Math.ceil(now / (server_period * 1000));
		return n_periods * server_period * 1000;
	}
}
