import Bullet.BulletGroup;
import Controller.KeyboardController;
import Player.GhostedPlayer;
import Types.Control;
import Types.ControlFrame;
import Types.EntityDesc;
import Types.Snapshot;
import Types.TileMap;
import Types.TileSet;
import Types.Weapon;
import Types.WorldConfig;
import Utils.Time;
import flash.display.BlendMode;
import flixel.FlxCamera;
import flixel.FlxG;
import flixel.FlxSprite;
import flixel.graphics.frames.FlxFilterFrames;
import flixel.group.FlxGroup;
import flixel.math.FlxRect;
import flixel.text.FlxText;
import flixel.tile.FlxTilemap;
import flixel.util.FlxColor;
import flixel.util.FlxSpriteUtil;
import haxe.io.BytesBuffer;
import openfl.filters.BlurFilter;
import seedyrng.Random;
import seedyrng.Xorshift64Plus;

class World extends FlxGroup
{
	#if debug
	var player:GhostedPlayer;
	#else
	var player:Player;
	#end

	var snapshot_history:Ring.PositionalRing<Snapshot>;
	var frame_history:Array<ControlFrame> = [];

	var interpolation_lag:Int = 30; // ms

	var world_camera:FlxCamera;
	var ui_camera:FlxCamera;

	var entities:FlxGroup;
	var entities_bullets:BulletGroup;
	var entities_bullet_tails:FlxTypedGroup<FlxSprite>;
	var entities_hud:HUD;

	var tilemap:TileMap;
	var level:Level;
	var minimap:Minimap;

	override public function new(config:WorldConfig, tilemap:TileMap, tileset:TileSet, weapons:haxe.ds.Vector<Weapon>)
	{
		super();

		trace("Initializing...");

		this.tilemap = tilemap;

		// init cameras
		world_camera = new FlxCamera(0, 0, Config.screen_width, Config.screen_height);
		world_camera.antialiasing = true;
		ui_camera = new FlxCamera(0, 0, Config.screen_width, Config.screen_height);
		ui_camera.antialiasing = true;

		world_camera.bgColor = 0xFF00AFB5;
		ui_camera.bgColor = FlxColor.TRANSPARENT;

		FlxG.cameras.reset(world_camera);
		FlxG.cameras.add(ui_camera);
		FlxCamera.defaultCameras = [world_camera];

		// groups

		entities = new FlxGroup();
		entities_bullets = new BulletGroup();
		entities_bullet_tails = new FlxTypedGroup<FlxSprite>();
		entities_hud = new HUD(world_camera);
		entities_hud.cameras = [ui_camera];

		// level

		level = new Level(tilemap, tileset);
		trace("loaded level " + level.toString());

		// minimap

		minimap = new Minimap(level, 300, 300);
		minimap.x = 5;
		minimap.y = Config.screen_height - minimap.height - 5;
		entities_hud.push(minimap);

		// weapons

		Weapons.weapons = weapons;
		trace("loaded weapons " + Weapons.toString());

		// init state

		#if debug
		player = new GhostedPlayer(config.snapshot.player.id, config.snapshot.player.radius, new KeyboardController(), FlxColor.RED, true);
		player.player.visible = true;
		FlxG.camera.follow(player.player, TOPDOWN, 1);
		entities_hud.push(player.player.hud, player.player);
		#else
		player = new Player(config.snapshot.player.id, config.snapshot.player.radius, new KeyboardController(), FlxColor.RED, true);
		player.visible = true;
		FlxG.camera.follow(player, TOPDOWN, 1);
		entities_hud.push(player.hud, player);
		#end
		player.rollback(config.snapshot.server_tick, config.snapshot.player, true);

		#if debug
		var weap_bar_hud = new HUD.WeaponBar(player.player);
		#else
		var weap_bar_hud = new HUD.WeaponBar(player);
		#end
		entities_hud.push(weap_bar_hud);

		snapshot_history = new Ring.PositionalRing<Snapshot>(Config.ACK_SIZE * 8);
		handleSnapshot(config.snapshot);

		// add everything in order
		add(level);
		add(entities_bullet_tails);
		add(entities_bullets);
		add(entities);
		add(player);
		add(entities_hud);

		trace("Starting at tick " + Time.nowInClientTicks());
		trace("Number of entities: " + Std.int(1 + entities.countLiving()));
		trace("Ready");
	}

	override public function update(elapsed:Float)
	{
		super.update(elapsed);

		var player_pos = player.getMiddlePoint();
		level.setPlayerPosition(player_pos.x, player_pos.y);

		var interp_tick:Float = Time.nowInClientTicks() - Time.msToClientTicks(interpolation_lag);

		for (entity in entities)
			(cast entity).interpolate(interp_tick);

		var last_snap = snapshot_history.get(0);
		player.rollback(last_snap.server_tick, last_snap.player, false);
		for (frame in frame_history)
		{
			player.replay(frame.client_tick, frame.control);
			// FlxG.collide(map, player);

			if (frame.control.shoot && player.canShoot(frame.client_tick))
			{
				for (i in 0...player.equipped_weapon.bullet_count)
				{
					var random_noise = player.random_generator.uniform(-player.equipped_weapon.spread / 180 * Math.PI / 2,
						player.equipped_weapon.spread / 180 * Math.PI / 2);
					var bullet = new Bullet(player.ID, player.x + player.radius, player.y + player.radius, player.facing_angle + random_noise,
						player.equipped_weapon.damage, player.equipped_weapon.range);
					entities_bullets.add(bullet);
					entities_bullet_tails.add(bullet.tail);
					var dist = doHitScan(frame.client_tick, bullet);
					if (dist > 0)
						bullet.setImpactDist(dist);
				}
				player.registerShoot(frame.client_tick);
			}
		}

		// for (bullet in entities_bullets)
		// {
		// 	map.overlapsWithCallback(bullet, function(_, bullet)
		// 	{
		// 		bullet.kill();
		// 		return false;
		// 	});
		// }

		// zoom
		// world_camera.zoom += FlxG.mouse.wheel * zoomSpeed;
		// if (world_camera.zoom > maxZoom)
		// 	world_camera.zoom = maxZoom;
		// else if (world_camera.zoom < minZoom)
		// 	world_camera.zoom = minZoom;

		// fullscreen
		if (FlxG.keys.anyJustPressed([Config.key_fullscreen]) && FlxG.keys.pressed.CONTROL)
			FlxG.fullscreen = !FlxG.fullscreen;
	}

	public function doHitScan(tick:Int, bullet:Bullet)
	{
		var closest_entity = null;
		var closest_dist = -1.0;

		for (e in entities)
		{
			#if debug
			var entity:GhostedPlayer = cast(e, GhostedPlayer);
			#else
			var entity:Entity = cast(e, Entity);
			#end
			var dist = Collisions.raySphereDistance(bullet.x, bullet.y, bullet.shoot_angle, entity.x + entity.radius, entity.y + entity.radius, entity.radius);
			if (dist > 0 && dist < bullet.range)
				if (closest_dist < 0 || dist < closest_dist)
				{
					closest_entity = entity;
					closest_dist = dist;
				}
		}

		var wall_dist = Collisions.rayMapDistance(bullet.x, bullet.y, bullet.shoot_angle, bullet.range, tilemap);

		if (closest_dist < 0 || wall_dist < closest_dist)
			return wall_dist;

		closest_entity.predict_damage(tick, bullet.damage);
		return closest_dist;
	}

	public function despawnEntities(snapshot:Snapshot)
	{
		for (id in snapshot.despawned)
		{
			var entity = getEntityById(id);
			if (entity != null)
				entity.kill();
		}
	}

	public function spawnEntity(tick:Int, desc:EntityDesc)
	{
		switch (desc.type)
		{
			case 2:
				{
					#if debug
					var player = GhostedPlayer.ofEntityDesc(tick, desc);
					entities.add(player);
					entities_hud.push(player.player.hud, player.player);
					#else
					var player = Player.ofEntityDesc(tick, desc);
					addPlayer(player);
					#end
				}
			default:
				{
					var entity = Entity.ofEntityDesc(tick, desc);
					addEntity(entity);
				}
		}
	}

	function addEntity(entity:Entity)
	{
		entities.add(entity);
		entities_hud.push(entity.hud, entity);
	}

	function addPlayer(player:Player)
	{
		addEntity(player);
	}

	function getEntityById(id:Int):Entity
	{
		for (entity in entities)
			if (entity.ID == id)
				return cast(entity);

		return null;
	}

	public function handleSnapshot(snapshot:Snapshot)
	{
		if (snapshot == null || snapshot_history.count() <= 0 || snapshot.server_tick > snapshot_history.get(0).server_tick)
		{
			snapshot_history.write(snapshot.id, snapshot);

			for (desc in snapshot.entities)
			{
				var entity = getEntityById(desc.id);
				if (entity == null)
					spawnEntity(snapshot.server_tick, desc);
				else
					(cast entity).register(snapshot.server_tick, desc);
			}
			despawnEntities(snapshot);

			while (frame_history.length > 0 && frame_history[0].client_tick <= snapshot.client_tick)
				frame_history.shift();
		}
	}

	public function makeControlFrame()
	{
		var frame = {
			client_tick: Math.floor(Time.nowInClientTicks()),
			last_snapshot: snapshot_history.get(0).id,
			ack: snapshot_history.makeAck(),
			control: player.getControl(),
		};
		frame_history.push(frame);

		return frame;
	}
}
