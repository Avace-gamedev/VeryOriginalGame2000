import Types.TileMap;
import Types.TileSet;
import flixel.FlxSprite;
import flixel.group.FlxGroup;
import flixel.math.FlxPoint;
import flixel.util.FlxColor;

class Level extends FlxTypedGroup<FlxSprite>
{
	public var tilemap:TileMap;
	public var tileset:TileSet;

	public var width:Int;
	public var height:Int;

	public var player_position:FlxPoint;

	var texture:FlxSprite;
	var light:FlxSprite;

	override public function new(tilemap:TileMap, tileset:TileSet)
	{
		super();
		this.tilemap = tilemap;
		this.tileset = tileset;

		width = tilemap.tile_size * tilemap.scale * tilemap.width;
		height = tilemap.tile_size * tilemap.scale * tilemap.height;

		player_position = new FlxPoint(0, 0);

		texture = new FlxSprite();
		texture.makeGraphic(width, height, FlxColor.TRANSPARENT);
		texture.shader = new Shaders.MapShader(tilemap, tileset);
		add(texture);

		light = new FlxSprite();
		light.makeGraphic(width, height, FlxColor.TRANSPARENT);
		light.blend = flash.display.BlendMode.MULTIPLY;
		light.shader = new Shaders.LightShader(tilemap, tileset);
		add(light);
	}

	public function setPlayerPosition(x:Float, y:Float)
	{
		this.player_position.set(x, y);
		cast(light.shader, Shaders.LightShader).setLightPos(x, y);
	}

	override public function toString()
	{
		return '[Level ' + width + 'x' + height + ']';
	}
}
