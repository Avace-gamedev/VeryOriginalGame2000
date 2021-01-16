import flixel.FlxCamera;
import flixel.FlxG;
import flixel.FlxSprite;
import flixel.group.FlxGroup;
import flixel.text.FlxText;
import flixel.util.FlxColor;
import flixel.util.FlxSpriteUtil;

class HUD extends FlxTypedGroup<FlxTypedGroup<FlxSprite>>
{
	var world_camera:FlxCamera;
	var targets:Array<FlxSprite> = [];

	override public function new(world_camera:FlxCamera)
	{
		super();
		this.world_camera = world_camera;
	}

	public function push(grp:FlxTypedGroup<FlxSprite>, target:FlxSprite = null)
	{
		if (grp == null)
			return;

		add(grp);

		if (target != null)
		{
			grp.ID = targets.length;
			targets.push(target);
		}
		else
			grp.ID = -1;
	}

	function getScreenPosition(target:FlxSprite)
	{
		var x = (target.x + target.width / 2 - world_camera.scroll.x) * world_camera.scaleX;
		var y = (target.y + target.height / 2 - world_camera.scroll.y) * world_camera.scaleY;

		return {x: x, y: y};
	}

	override public function update(elapsed:Float)
	{
		super.update(elapsed);

		for (grp in this)
		{
			if (grp.ID >= 0)
			{
				var follow_pos = getScreenPosition(targets[grp.ID]);
				for (elt in grp)
				{
					elt.x += follow_pos.x;
					elt.y += follow_pos.y;
				}
			}
		}
	}
}

class WeaponBar extends FlxTypedGroup<FlxSprite>
{
	var text_prev:FlxText;
	var text_next:FlxText;

	var text_prev_timer:Float = -1;
	var text_next_timer:Float = -1;

	var weap_bar:FlxSprite;
	var selected_weap:FlxSprite;
	var player:Player;

	public var width:Int;
	public var height:Int;

	override public function new(player:Player)
	{
		super();
		this.player = player;

		width = Config.MAX_N_WEAPONS * Config.HUD_weap_width + (Config.MAX_N_WEAPONS + 1) * Config.HUD_weap_hsep;
		height = Config.HUD_weap_height + 2 * Config.HUD_weap_vsep;

		weap_bar = new FlxSprite();
		makeWeaponBar();
		add(weap_bar);

		selected_weap = new FlxSprite();
		selected_weap.makeGraphic(Config.HUD_weap_width + 2 * Config.HUD_weap_hsep, Config.HUD_weap_height + 2 * Config.HUD_weap_vsep, FlxColor.TRANSPARENT);
		FlxSpriteUtil.drawRect(selected_weap, Config.HUD_weap_hsep, Config.HUD_weap_vsep, Config.HUD_weap_width, Config.HUD_weap_height, FlxColor.TRANSPARENT,
			{
				thickness: 3,
				color: FlxColor.fromRGBFloat(0, 0, 0, 0.5),
			});
		selected_weap.y = weap_bar.y;
		add(selected_weap);

		text_prev = new FlxText(0, 0, 0, Config.key_prev_weap.toString());
		text_prev.setFormat(Config.HUD_weap_font, Config.HUD_weap_font_size, FlxColor.BLACK, FlxTextAlign.RIGHT);
		text_prev.x = weap_bar.x - text_prev.width;
		text_prev.y = weap_bar.y + (height - text_prev.height) / 2;
		add(text_prev);

		text_next = new FlxText(0, 0, 0, Config.key_next_weap.toString());
		text_next.setFormat(Config.HUD_weap_font, Config.HUD_weap_font_size, FlxColor.BLACK, FlxTextAlign.LEFT);
		text_next.x = weap_bar.x + width;
		text_next.y = weap_bar.y + (height - text_next.height) / 2;
		add(text_next);
	}

	public function makeWeaponBar()
	{
		var n_weapons = player.nWeapons();
		weap_bar.makeGraphic(width, height, FlxColor.TRANSPARENT);
		for (i in 0...Config.MAX_N_WEAPONS)
			FlxSpriteUtil.drawRect(weap_bar, Config.HUD_weap_hsep + i * (Config.HUD_weap_width + Config.HUD_weap_hsep), Config.HUD_weap_vsep,
				Config.HUD_weap_width, Config.HUD_weap_height, FlxColor.TRANSPARENT, {
				color: i < n_weapons ? FlxColor.fromRGBFloat(0, 0, 0, 0.5) : FlxColor.fromRGBFloat(0, 0, 0, 0.2)
			});
		weap_bar.x = (Config.screen_width - width) / 2;
		weap_bar.y = Config.screen_height - height;
	}

	override public function update(elapsed:Float)
	{
		if (text_prev_timer > 0)
		{
			if (text_prev_timer < elapsed)
				text_prev.setFormat(Config.HUD_weap_font, Config.HUD_weap_font_size, FlxColor.BLACK, FlxTextAlign.RIGHT);
			text_prev_timer -= elapsed;
		}

		if (text_next_timer > 0)
		{
			if (text_next_timer < elapsed)
				text_next.setFormat(Config.HUD_weap_font, Config.HUD_weap_font_size, FlxColor.BLACK, FlxTextAlign.LEFT);
			text_next_timer -= elapsed;
		}

		if (FlxG.keys.anyJustPressed([Config.key_prev_weap]))
		{
			text_prev.setFormat(Config.HUD_weap_font, Config.HUD_weap_font_size, FlxColor.RED, FlxTextAlign.RIGHT);
			text_prev_timer = Config.HUD_weap_duration;
		}

		if (FlxG.keys.anyJustPressed([Config.key_next_weap]))
		{
			text_next.setFormat(Config.HUD_weap_font, Config.HUD_weap_font_size, FlxColor.RED, FlxTextAlign.LEFT);
			text_next_timer = Config.HUD_weap_duration;
		}

		selected_weap.x = weap_bar.x + player.weapon_i * (Config.HUD_weap_width + Config.HUD_weap_hsep);

		super.update(elapsed);
	}
}
