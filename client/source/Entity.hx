import Types.EntityDesc;
import Types.Snapshot;
import Types.Timed;
import flixel.FlxSprite;
import flixel.group.FlxGroup;
import flixel.text.FlxText;
import flixel.ui.FlxBar;
import flixel.util.FlxColor;
import flixel.util.FlxSpriteUtil;

enum EntityType
{
	ENTITY;
	PLAYER;
}

class Entity extends FlxSprite
{
	public var hud:EntityHUD;

	public var name:String = "__entity__";
	public var type:EntityType;
	public var radius(default, set):Float = -1;

	public var max_health:Float;
	public var predicted_health:Float;
	public var server_health:Float;
	public var restore_server_health:Float = 0;

	var last_seen_tick = 0;
	var initial_color:FlxColor;

	var timeline:Ring.Ring<Timed<EntityDesc>>;

	public function new(id:Int, radius:Float, type:EntityType = ENTITY, color = FlxColor.GREEN, hud:EntityHUD = null)
	{
		super();

		timeline = new Ring.Ring<Timed<EntityDesc>>(Config.ACK_SIZE * 8);

		this.ID = id;
		this.type = type;
		this.initial_color = color;
		this.color = color;
		this.radius = radius;
		this.visible = false;

		this.hud = hud;
	}

	override public function update(elapsed:Float)
	{
		var tick = Utils.Time.nowInClientTicks();

		if (tick > restore_server_health)
			predicted_health = server_health;

		if (last_seen_tick < -Config.end_forget_entity)
			color = FlxColor.fromRGBFloat(0.3, 0.3, 0.3, 0.3);
		else if (last_seen_tick < tick - Config.start_forget_entity)
		{
			var coef = (tick - Config.start_forget_entity - last_seen_tick) / (Config.end_forget_entity - Config.start_forget_entity);
			color = FlxColor.fromRGBFloat(initial_color.redFloat * (1 - coef)
				+ 0.3 * coef, initial_color.greenFloat * (1 - coef)
				+ 0.3 * coef,
				initial_color.blueFloat * (1 - coef)
				+ 0.3 * coef, 0.7 * (1 - coef)
				+ 0.3 * coef);

			if (hud != null)
				hud.show_health = false;
		}
		else
		{
			color = initial_color;

			if (hud != null)
				hud.show_health = true;
		}

		super.update(elapsed);
	}

	public function setHUD(hud:EntityHUD)
	{
		this.hud = hud;
	}

	public function set_radius(radius:Float)
	{
		if (Math.floor(this.radius) != Math.floor(radius))
		{
			makeGraphic(Math.floor(2 * radius), Math.floor(2 * radius), FlxColor.TRANSPARENT, true);
			FlxSpriteUtil.drawCircle(this, radius, radius, radius, FlxColor.WHITE);
			this.radius = radius;
		}
		return this.radius;
	}

	public function rollback(tick:Int, desc:EntityDesc, first = false)
	{
		last_seen_tick = tick;

		ID = desc.id;
		name = desc.name;
		x = desc.x;
		y = desc.y;
		server_health = desc.health;
		if (server_health < predicted_health)
			predicted_health = server_health;
		max_health = desc.max_health;
		radius = desc.radius;

		if (first)
			predicted_health = desc.health;
	}

	public function predict_damage(tick:Int, damage:Float)
	{
		predicted_health -= damage;
		restore_server_health = tick + Config.restore_server_health_after;
	}

	public function interpolate(tick:Float)
	{
		var timed_desc = Utils.interpolateRingTimedDesc(timeline, tick);

		if (timed_desc == null)
		{
			visible = false;
			return null;
		}

		visible = true;
		rollback(Math.floor(timed_desc.tick), timed_desc.val, false);

		return timed_desc.val;
	}

	public function register(tick:Float, desc:EntityDesc)
		timeline.push({tick: tick, val: desc});

	public static function ofEntityDesc(tick:Int, desc:EntityDesc):Entity
	{
		var entity = new Entity(desc.id, desc.radius, ENTITY);
		entity.rollback(tick, desc, true);
		return entity;
	}
}

class EntityHUD extends FlxTypedGroup<FlxSprite>
{
	var entity:Entity;

	var name_tag:FlxText;

	public var show_health:Bool = true;

	var health_bar:FlxBar;
	var predicted_health_bar:FlxBar;

	override public function new(entity:Entity)
	{
		super();
		this.entity = entity;

		name_tag = new FlxText(0, 0, 0, entity.name);
		name_tag.setFormat(Config.name_tag_font, Config.name_tag_font_size, FlxColor.BLACK);
		add(name_tag);

		health_bar = new FlxBar(0, 0, LEFT_TO_RIGHT, Config.health_bar_width, Config.health_bar_height, "", 0, entity.max_health, true);
		health_bar.createColoredEmptyBar(FlxColor.BLACK, false);
		health_bar.createColoredFilledBar(FlxColor.YELLOW, true, FlxColor.BLACK);
		add(health_bar);

		predicted_health_bar = new FlxBar(0, 0, LEFT_TO_RIGHT, Config.health_bar_width, Config.health_bar_height, "", 0, entity.max_health, true);
		predicted_health_bar.createColoredEmptyBar(FlxColor.TRANSPARENT);
		predicted_health_bar.createGradientFilledBar([FlxColor.GREEN, FlxColor.YELLOW, FlxColor.RED], 1, 180, true, FlxColor.BLACK);
		add(predicted_health_bar);
	}

	override public function update(elapsed:Float)
	{
		super.update(elapsed);

		name_tag.x = -name_tag.width / 2;
		name_tag.y = -entity.radius - (show_health ? health_bar.height : 0) - name_tag.height - Config.h_offset;
		name_tag.text = entity.name;

		health_bar.x = -health_bar.width / 2;
		health_bar.y = -entity.radius - health_bar.height - Config.h_offset;
		health_bar.value = entity.server_health;
		health_bar.setRange(0, entity.max_health);
		health_bar.visible = show_health;

		predicted_health_bar.x = -predicted_health_bar.width / 2;
		predicted_health_bar.y = -entity.radius - predicted_health_bar.height - Config.h_offset;
		predicted_health_bar.value = entity.predicted_health;
		predicted_health_bar.setRange(0, entity.max_health);
		predicted_health_bar.visible = show_health;
	}
}
