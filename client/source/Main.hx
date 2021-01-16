package;

import flixel.FlxG;
import flixel.FlxGame;
import openfl.display.Sprite;

class Main extends Sprite
{
	public function new()
	{
		super();
		addChild(new FlxGame(Config.screen_width, Config.screen_height, WelcomeState, 1, 60, 60, true));
	}
}
