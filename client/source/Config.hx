import flixel.input.keyboard.FlxKey;
import flixel.util.FlxColor;

class Config
{
	public static var ACK_SIZE:Int = 2;
	public static var CLIENT_RATE:Int = 60;
	public static var SERVER_RATE:Int = 20;

	public static var MAX_N_WEAPONS:Int = 4;

	public static var some_names:Array<String> = ["Mr. le Bref", "Duc Dérénaze", "Anastue", "Pashakina", "Sékiki Pudoucu"];

	public static var server_ip:String = "127.0.0.1";
	public static var server_port:Int = 8890;

	public static var XORSHIFT64PLUS_STATE_SIZE = 16;

	public static var OP_PING(default, never):Int = 1;
	public static var OP_PONG(default, never):Int = 2;
	public static var OP_BINARY(default, never):Int = 5;

	public static var OP_INIT(default, never):Int = 11;
	public static var OP_STATIC_INFO(default, never):Int = 12;
	public static var OP_CONFIG(default, never):Int = 13;
	public static var OP_CLIENT_READY(default, never):Int = 14;

	public static var OP_SNAPSHOT(default, never):Int = 100;
	public static var OP_CONTROL_FRAME(default, never):Int = 101;

	public static var screen_width:Int = 1920;
	public static var screen_height:Int = 1080;

	public static var h_offset = 5;
	public static var v_offset = 5;

	public static var name_tag_font = AssetPaths.Lato_Regular__ttf;
	public static var name_tag_font_size = 16;
	public static var health_bar_width = 50;
	public static var health_bar_height = 5;

	public static var HUD_weap_vsep = 10;
	public static var HUD_weap_hsep = 20;
	public static var HUD_weap_width = 50;
	public static var HUD_weap_height = 50;
	public static var HUD_weap_font = null;
	public static var HUD_weap_font_size = 20;
	public static var HUD_weap_duration = 0.05;

	public static var hit_scan_bullet_lifetime = 0.1;
	public static var restore_server_health_after = 60; // ticks
	public static var start_forget_entity = 30;
	public static var end_forget_entity = 150;

	public static var bullet_color:FlxColor = FlxColor.GRAY;
	public static var bullet_tail_color:FlxColor = FlxColor.GRAY;

	public static var key_up:FlxKey = FlxKey.Z;
	public static var key_down:FlxKey = FlxKey.S;
	public static var key_left:FlxKey = FlxKey.Q;
	public static var key_right:FlxKey = FlxKey.D;
	public static var key_weap:Array<FlxKey> = [FlxKey.ONE, FlxKey.TWO, FlxKey.THREE];
	public static var key_next_weap:FlxKey = FlxKey.E;
	public static var key_prev_weap:FlxKey = FlxKey.A;
	public static var key_run:FlxKey = FlxKey.SHIFT;
	public static var key_fullscreen:FlxKey = FlxKey.F;

	//
	public static var walk_speed_mod:Float = 0.7;
	public static var change_weapon_delay_tick:Float = 30;
}
