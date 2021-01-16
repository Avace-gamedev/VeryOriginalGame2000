import Types.TileMap;
import Types.TileSet;
import flixel.FlxSprite;
import flixel.group.FlxGroup;
import flixel.math.FlxRect;
import flixel.util.FlxColor;
import flixel.util.FlxSpriteUtil;

class Minimap extends FlxTypedGroup<FlxSprite>
{
	var level:Level;
	var map:FlxSprite;
	var light:FlxSprite;
	var frame:FlxSprite;
	var player:FlxSprite;
	var player_radius:Int = 2;

	public var width:Int;
	public var height:Int;
	public var x(get, set):Float;
	public var y(get, set):Float;

	var border_size:Int = 4;
	var min_clip_x:Int;
	var min_clip_y:Int;
	var max_clip_x:Int;
	var max_clip_y:Int;
	var clip_w:Int;
	var clip_h:Int;

	override public function new(level:Level, width:Int, height:Int)
	{
		super();
		this.level = level;
		this.width = width;
		this.height = height;

		map = new FlxSprite();
		map.makeGraphic(level.tilemap.tile_size * level.tilemap.width, level.tilemap.tile_size * level.tilemap.height, FlxColor.TRANSPARENT);
		map.shader = new Shaders.MapShader(level.tilemap, level.tileset);

		light = new FlxSprite();
		light.makeGraphic(level.tilemap.tile_size * level.tilemap.width, level.tilemap.tile_size * level.tilemap.height, FlxColor.TRANSPARENT);
		light.blend = flash.display.BlendMode.MULTIPLY;
		light.shader = new Shaders.LightShader(level.tilemap, level.tileset);

		frame = new FlxSprite();
		frame.makeGraphic(width, height, FlxColor.TRANSPARENT);
		FlxSpriteUtil.drawRect(frame, 0, 0, width, height, FlxColor.BLACK);

		player = new FlxSprite();
		player.makeGraphic(2 * player_radius, 2 * player_radius, FlxColor.TRANSPARENT);
		FlxSpriteUtil.drawCircle(player, player_radius, player_radius, player_radius, FlxColor.RED);

		add(frame);
		add(map);
		add(light);
		add(player);

		clip_w = width - 2 * border_size;
		clip_h = height - 2 * border_size;
		min_clip_x = 0;
		min_clip_y = 0;
		max_clip_x = Math.floor(map.width - clip_w);
		max_clip_y = Math.floor(map.height - clip_h);
	}

	function get_x()
	{
		return frame.x;
	}

	function get_y()
	{
		return frame.y;
	}

	function set_x(x:Float)
	{
		frame.x = x;
		return x;
	}

	function set_y(y:Float)
	{
		frame.y = y;
		return y;
	}

	override public function update(elapsed)
	{
		super.update(elapsed);

		var player_pos_x = level.player_position.x / level.tilemap.scale;
		var player_pos_y = level.player_position.y / level.tilemap.scale;

		cast(light.shader, Shaders.LightShader).setLightPos(player_pos_x, player_pos_y);

		var rect_x = Math.min(max_clip_x, Math.max(min_clip_x, player_pos_x - width / 2 - border_size));
		var rect_y = Math.min(max_clip_y, Math.max(min_clip_y, player_pos_y - height / 2 - border_size));
		map.clipRect = new FlxRect(rect_x, rect_y, clip_w, clip_h);
		map.x = frame.x - rect_x + border_size;
		map.y = frame.y - rect_y + border_size;

		light.clipRect = new FlxRect(rect_x, rect_y, clip_w, clip_h);
		light.x = frame.x - rect_x + border_size;
		light.y = frame.y - rect_y + border_size;

		player.x = frame.x - rect_x + player_pos_x - player_radius;
		player.y = frame.y - rect_y + player_pos_y - player_radius;
	}
}
