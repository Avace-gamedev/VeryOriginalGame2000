import Types.Control;
import Types.Movement;
import flixel.FlxG;

class Controller
{
	var player:Player;

	public function new() {}

	public function assign(player:Player)
	{
		this.player = player;
	}

	public function getControl()
		return null;
}

class NoController extends Controller {}

class KeyboardController extends Controller
{
	override public function new()
	{
		super();
	}

	override public function getControl()
	{
		if (player == null)
			return null;

		var movement = [];

		if (FlxG.keys.anyPressed([Config.key_up]))
			movement.push(UP);
		if (FlxG.keys.anyPressed([Config.key_down]))
			movement.push(DOWN);
		if (FlxG.keys.anyPressed([Config.key_left]))
			movement.push(LEFT);
		if (FlxG.keys.anyPressed([Config.key_right]))
			movement.push(RIGHT);

		var new_weapon_i = -1;
		var n_weapons = player.nWeapons();
		if (FlxG.keys.anyJustPressed([Config.key_next_weap]))
			new_weapon_i = (player.weapon_i + 1) % n_weapons;
		if (FlxG.keys.anyJustPressed([Config.key_prev_weap]))
			new_weapon_i = (player.weapon_i + n_weapons - 1) % n_weapons;

		for (i in 0...Config.key_weap.length)
			if (FlxG.keys.anyJustPressed([Config.key_weap[i]]) && i < n_weapons)
				new_weapon_i = i;

		var shoot = FlxG.mouse.pressed;
		var run = FlxG.keys.anyPressed([Config.key_run]);

		var mX = FlxG.mouse.x;
		var mY = FlxG.mouse.y;
		var facing_angle = Math.atan2(mY - player.y, mX - player.x);

		return {
			movement: movement,
			change_weapon: new_weapon_i >= 0,
			new_weapon_i: new_weapon_i > 0 ? new_weapon_i : 0,
			run: run,
			shoot: shoot,
			facing_angle: facing_angle,
		};
	}
}
