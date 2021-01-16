import flixel.FlxSprite;
import flixel.group.FlxGroup;
import flixel.util.FlxColor;
import flixel.util.FlxGradient;
import flixel.util.FlxSpriteUtil;

class Bullet extends FlxSprite
{
	public var owner:Int;
	public var shoot_angle:Float;
	public var damage:Float;
	public var radius(default, set):Float = -1;
	public var tail_length:Float = 40;
	public var tail_thickness:Int = 2;
	public var range:Float;
	public var impact_dist:Float = -1;

	var init_pos_x:Float = 0;
	var init_pos_y:Float = 0;

	public var tail:FlxSprite;

	override public function new(owner:Int, x:Float, y:Float, angle:Float, damage:Float = 0, range:Float = 500)
	{
		super();
		this.radius = 1;
		this.owner = owner;
		this.x = x;
		this.y = y;
		this.init_pos_x = x;
		this.init_pos_y = y;
		this.shoot_angle = angle;
		this.damage = damage;
		this.range = range;
		this.solid = true;
		this.visible = true;
		this.immovable = true;
		shoot();

		tail = new FlxSprite();
		tail.makeGraphic(Math.ceil(tail_length), tail_thickness, FlxColor.TRANSPARENT, true);
		tail.angle = (angle - Math.PI) * 180 / Math.PI;
		tail.origin.set(0, tail_thickness / 2);
	}

	public function set_radius(radius:Float)
	{
		if (Math.floor(this.radius) != Math.floor(radius))
		{
			makeGraphic(Math.floor(2 * radius), Math.floor(2 * radius), FlxColor.TRANSPARENT, true);
			FlxSpriteUtil.drawCircle(this, radius, radius, radius, Config.bullet_color);
			this.radius = radius;
		}
		return this.radius;
	}

	override public function update(elapsed:Float)
	{
		super.update(elapsed);

		tail.x = x + radius;
		tail.y = y + radius - tail_thickness / 2;

		var dist_traveled = Math.sqrt((x - init_pos_x) * (x - init_pos_x) + (y - init_pos_y) * (y - init_pos_y));
		alpha = 1 - dist_traveled / range;

		FlxSpriteUtil.fill(tail, FlxColor.TRANSPARENT);
		FlxGradient.overlayGradientOnFlxSprite(tail, Math.floor(Math.min(tail_length, dist_traveled)), tail_thickness, [
			FlxColor.fromRGBFloat(Config.bullet_tail_color.red, Config.bullet_tail_color.green, Config.bullet_tail_color.blue, alpha),
			FlxColor.TRANSPARENT,
		], 0, 0, 1, 180);

		if (dist_traveled >= range || impact_dist > 0 && dist_traveled >= impact_dist)
			kill();
	}

	public function setImpactDist(dist:Float)
	{
		impact_dist = dist;
	}

	override public function kill()
	{
		super.kill();
		tail.kill();
	}

	public function shoot()
	{
		var speed = range / Config.hit_scan_bullet_lifetime;
		this.velocity.x = Math.cos(shoot_angle) * speed;
		this.velocity.y = Math.sin(shoot_angle) * speed;
	}
}

class BulletGroup extends FlxTypedGroup<Bullet> {}
