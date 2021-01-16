package;

import flixel.FlxG;
import flixel.addons.ui.FlxUIBar;
import flixel.addons.ui.FlxUISubState;
import flixel.addons.ui.FlxUIText;
import flixel.util.FlxColor;

class LoadingState extends FlxUISubState
{
	public var min_progress:Float = 0;
	public var max_progress:Float = 100;
	public var progress:Float = 0;

	override public function create()
	{
		FlxG.autoPause = false;
		FlxG.cameras.bgColor = FlxColor.TRANSPARENT;
		_xml_id = "loading";
		super.create();
	}

	override public function update(elapsed:Float)
	{
		super.update(elapsed);

		var percent = (progress - min_progress) / (max_progress - min_progress) * 100;

		var bar = cast(_ui.getAsset("progress_bar"), FlxUIBar);
		bar.value = percent;

		var text = cast(_ui.getAsset("progress_text"), FlxUIText);
		text.text = '$percent%';
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
						case "cancel":
							FlxG.switchState(new WelcomeState());
					}
		}
	}
}
