import Bullet.BulletGroup;
import Entity.EntityHUD;
import Types.Control;
import Types.EntityDesc;
import Types.Snapshot;
import Types.Timed;
import Types.Weapon;
import flixel.FlxSprite;
import flixel.group.FlxGroup;
import flixel.math.FlxPoint;
import flixel.util.FlxColor;
import flixel.util.FlxSpriteUtil;
import seedyrng.Xorshift64Plus;

class Player extends Entity
{
	var controller:Controller;

	public var random_generator:seedyrng.Random;

	public var move_speed:Float = 0;
	public var running:Bool = false;
	public var facing_angle:Float = 0;

	public var weapons:haxe.ds.Vector<Int>;
	public var weapon_i:Int = -1;
	public var equipped_weapon:Weapon;

	public var sight_length = 15;

	var tick_next_shoot(default, set):Float = 0;

	public function new(id:Int, radius:Float, controller:Controller = null, color:FlxColor = FlxColor.RED, show_sight:Bool = false)
	{
		super(id, radius, PLAYER, color);

		weapons = new haxe.ds.Vector<Int>(Config.MAX_N_WEAPONS);
		if (Weapons.weapons.length > 0)
		{
			weapons[0] = 0;
			weapon_i = 0;
			equipped_weapon = Weapons.get(0);
		}
		else
			trace("ERROR: no weapon found");

		setHUD(new PlayerHUD(this, show_sight));

		random_generator = new seedyrng.Random(new seedyrng.Xorshift64Plus());

		this.solid = true;

		this.controller = controller;
		if (controller != null)
			controller.assign(this);
	}

	function set_tick_next_shoot(new_value:Float)
	{
		// can't go back in time
		if (new_value > tick_next_shoot)
			tick_next_shoot = new_value;
		return tick_next_shoot;
	}

	public function nWeapons()
	{
		for (i in 0...weapons.length)
			if (weapons[i] < 0)
				return i;
		return 0;
	}

	public static function ofEntityDesc(tick:Int, desc:Types.EntityDesc, color = FlxColor.BLUE)
	{
		var player = new Player(desc.id, desc.radius, null, color);
		player.rollback(tick, desc, true);
		return player;
	}

	override public function rollback(tick:Int, desc:Types.EntityDesc, first = false)
	{
		super.rollback(tick, desc, first);
		weapons = desc.weapons;
		if (weapon_i != desc.weapon_i)
			equipped_weapon = Weapons.get(weapons[desc.weapon_i]);
		weapon_i = desc.weapon_i;
		move_speed = desc.move_speed;

		random_generator.state = desc.random_state;
	}

	public function canShoot(tick:Int)
	{
		return tick > tick_next_shoot;
	}

	public function registerShoot(tick:Int)
	{
		tick_next_shoot = tick + Utils.Time.msToClientTicks(1000 / equipped_weapon.rate);
	}

	public function replay(tick:Int, control:Control)
	{
		var move_x = 0;
		var move_y = 0;
		for (move in control.movement)
		{
			switch (move)
			{
				case UP:
					move_y -= 1;
				case DOWN:
					move_y += 1;
				case LEFT:
					move_x -= 1;
				case RIGHT:
					move_x += 1;
			}
		}

		running = control.run;
		var speed = running ? move_speed : move_speed * Config.walk_speed_mod;

		var move_norm = Math.sqrt(move_x * move_x + move_y * move_y);
		if (move_norm > 0)
		{
			x += speed * move_x / move_norm;
			y += speed * move_y / move_norm;
		}

		facing_angle = control.facing_angle;

		if (control.change_weapon)
		{
			weapon_i = control.new_weapon_i;
			equipped_weapon = Weapons.get(weapons[weapon_i]);
			tick_next_shoot = tick + Config.change_weapon_delay_tick;
		}
	}

	override public function interpolate(tick:Float)
	{
		var desc = super.interpolate(tick);

		if (desc != null)
			move_speed = desc.move_speed;

		return desc;
	}

	public function getControl()
	{
		if (this.controller != null)
			return controller.getControl();
		else
			return null;
	}

	public function getMiddlePoint()
	{
		return new FlxPoint(x + radius, y + radius);
	}
}

class PlayerHUD extends EntityHUD
{
	var player:Player;

	var sight_sprite_w:Int;
	var sight_sprite_h:Int;
	var sight_sprite:FlxSprite;

	var show_sight:Bool;

	override public function new(player:Player, show_sight:Bool)
	{
		super(player);

		this.player = player;
		this.show_sight = show_sight;

		if (show_sight)
		{
			sight_sprite = new FlxSprite();
			makeSight();
			add(sight_sprite);
		}
	}

	function makeSight()
	{
		var theta = player.equipped_weapon.spread / 180 * Math.PI;
		sight_sprite_w = Math.floor(Math.cos(theta / 2) * player.sight_length);
		sight_sprite_h = Math.floor(2 * Math.sin(theta / 2) * player.sight_length) + 1;

		sight_sprite.makeGraphic(sight_sprite_w, sight_sprite_h, FlxColor.TRANSPARENT, true);
		sight_sprite.solid = false;
		sight_sprite.immovable = true;

		var top_sight_start_x = player.radius * Math.cos(-theta / 2);
		var top_sight_start_y = sight_sprite_h / 2 + player.radius * Math.sin(-theta / 2);
		var bot_sight_start_x = player.radius * Math.cos(theta / 2);
		var bot_sight_start_y = sight_sprite_h / 2 + player.radius * Math.sin(theta / 2);
		FlxSpriteUtil.drawLine(sight_sprite, top_sight_start_x, top_sight_start_y, sight_sprite_w, 0, {
			color: FlxColor.BLACK
		});
		FlxSpriteUtil.drawLine(sight_sprite, bot_sight_start_x, bot_sight_start_y, sight_sprite_w, sight_sprite_h - 1, {
			color: FlxColor.BLACK
		});

		sight_sprite.origin.set(0, sight_sprite_h / 2);
	}

	override function update(elapsed:Float)
	{
		if (show_sight)
		{
			sight_sprite.x = 0;
			sight_sprite.y = -sight_sprite_h / 2;
			sight_sprite.angle = player.facing_angle * 180 / Math.PI;
		}

		super.update(elapsed);
	}
}

class GhostedPlayer extends FlxTypedGroup<Entity>
{
	public var player:Player;

	var ghost:Entity;

	public var random_generator(get, never):seedyrng.Random;

	public var hud(get, never):EntityHUD;
	public var name(get, never):String;
	public var x(get, set):Float;
	public var y(get, set):Float;
	public var radius(get, set):Float;
	public var facing_angle(get, set):Float;
	public var predicted_health(get, never):Float;
	public var server_health(get, never):Float;
	public var equipped_weapon(get, never):Weapon;
	public var move_speed(get, set):Float;

	override public function new(id:Int, radius:Float, controller:Controller = null, color = FlxColor.RED, show_sight:Bool = false)
	{
		super();

		this.ID = id;

		player = new Player(id, radius, controller, color, show_sight);
		ghost = new Entity(-1, radius, ENTITY, player.color.getLightened(0.5));
		ghost.visible = true;
		ghost.solid = false;
		ghost.alpha = 0.5;

		add(ghost);
		add(player);
	}

	function get_name()
		return player.name;

	function get_x()
		return player.x;

	function get_y()
		return player.y;

	function get_move_speed()
		return player.move_speed;

	function get_radius()
		return player.radius;

	function get_facing_angle()
		return player.facing_angle;

	function get_predicted_health()
		return player.predicted_health;

	function get_server_health()
		return player.server_health;

	function get_equipped_weapon()
		return player.equipped_weapon;

	function get_random_generator()
		return player.random_generator;

	function get_hud()
		return player.hud;

	function set_x(x:Float)
	{
		player.x = x;
		ghost.x = x;
		return x;
	}

	function set_y(y:Float)
	{
		player.y = y;
		ghost.y = y;
		return y;
	}

	function set_radius(radius:Float)
	{
		player.radius = radius;
		ghost.radius = radius;
		return radius;
	}

	function set_facing_angle(facing_angle:Float)
	{
		player.facing_angle = facing_angle;
		return facing_angle;
	}

	function set_move_speed(move_speed:Float)
		return player.move_speed = move_speed;

	public static function ofEntityDesc(tick:Int, desc:Types.EntityDesc, color = FlxColor.BLUE)
	{
		var player = new GhostedPlayer(desc.id, desc.radius, null, color);
		player.rollback(tick, desc, true);
		return player;
	}

	public function rollback(tick:Int, desc:Types.EntityDesc, first = false)
	{
		ghost.rollback(tick, desc, first);
		return player.rollback(tick, desc, first);
	}

	public function replay(tick:Int, control:Control)
		return player.replay(tick, control);

	public function register(tick:Float, desc:EntityDesc)
	{
		ghost.rollback(Math.floor(tick), desc, false);
		return player.register(tick, desc);
	}

	public function interpolate(tick:Float)
		return player.interpolate(tick);

	public function getControl()
		return player.getControl();

	public function canShoot(tick:Int)
		return player.canShoot(tick);

	public function registerShoot(tick:Int)
		return player.registerShoot(tick);

	public function predict_damage(tick:Int, damage:Float)
		return player.predict_damage(tick, damage);

	public function getMiddlePoint()
		return player.getMiddlePoint();
}
