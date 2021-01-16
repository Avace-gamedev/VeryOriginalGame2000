package;

import flixel.FlxG;
import flixel.addons.ui.FlxInputText;
import flixel.addons.ui.FlxUIState;
import flixel.util.FlxColor;

class ConnectionState extends FlxUIState
{
	override public function create()
	{
		FlxG.autoPause = false;
		FlxG.cameras.bgColor = 0xFF00AFB5;
		_xml_id = "connection";
		super.create();
	}

	override public function getEvent(name:String, sender:Dynamic, data:Dynamic, ?params:Array<Dynamic>)
	{
		switch (name)
		{
			case "finish_load":
				(cast(_ui.getAsset("input_address_1"), FlxInputText)).maxLength = 4;
				(cast(_ui.getAsset("input_address_2"), FlxInputText)).maxLength = 4;
				(cast(_ui.getAsset("input_address_3"), FlxInputText)).maxLength = 4;
				(cast(_ui.getAsset("input_address_4"), FlxInputText)).maxLength = 4;
				(cast(_ui.getAsset("input_address_port"), FlxInputText)).maxLength = 6;

				preFillAddress();

				cast(_ui.getAsset("name_field"), FlxInputText).text = Config.some_names[Std.random(Config.some_names.length)];
			case "click_button":
				if (params != null && params.length > 0)
					switch (Std.string(params[0]))
					{
						case "connect":
							Config.server_ip = getIp();
							Config.server_port = getPort();

							var name = cast(_ui.getAsset("name_field"), FlxInputText).text;
							if (name.length > 0)
							{
								trace("Connecting to " + Config.server_ip + ":" + Config.server_port);
								FlxG.switchState(new PlayState(name));
							}
						case "back":
							FlxG.switchState(new WelcomeState());
					}
		}
	}

	function preFillAddress()
	{
		var addrs = Config.server_ip.split('.');

		if (addrs.length > 0)
			(cast(_ui.getAsset("input_address_1"), FlxInputText)).text = addrs[0];
		if (addrs.length > 1)
			(cast(_ui.getAsset("input_address_2"), FlxInputText)).text = addrs[1];
		if (addrs.length > 2)
			(cast(_ui.getAsset("input_address_3"), FlxInputText)).text = addrs[2];
		if (addrs.length > 3)
			(cast(_ui.getAsset("input_address_4"), FlxInputText)).text = addrs[3];

		(cast(_ui.getAsset("input_address_port"), FlxInputText)).text = Std.string(Config.server_port);
	}

	function getIp()
	{
		var addr1 = (cast(_ui.getAsset("input_address_1"), FlxInputText)).text;
		var addr2 = (cast(_ui.getAsset("input_address_2"), FlxInputText)).text;
		var addr3 = (cast(_ui.getAsset("input_address_3"), FlxInputText)).text;
		var addr4 = (cast(_ui.getAsset("input_address_4"), FlxInputText)).text;
		return '$addr1.$addr2.$addr3.$addr4';
	}

	function getPort()
	{
		var port = (cast _ui.getAsset("input_address_port")).text;
		return Std.parseInt(port);
	}
}
