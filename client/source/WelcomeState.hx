package;

import flixel.FlxG;
import flixel.addons.ui.FlxUIState;

class WelcomeState extends FlxUIState
{
	override public function create()
	{
		FlxG.autoPause = false;
		FlxG.cameras.bgColor = 0xFF00AFB5;
		_xml_id = "welcome";
		super.create();
	}

	override public function getEvent(name:String, sender:Dynamic, data:Dynamic, ?params:Array<Dynamic>)
	{
		switch (name)
		{
			case "finish_load":
			case "click_button":
				if (params != null && params.length > 0)
					switch (Std.string(params[0]))
					{
						case "launch":
							FlxG.switchState(new ConnectionState());
						case "settings":
							FlxG.switchState(new SettingsState());
					}
		}
	}
}
