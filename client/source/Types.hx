typedef Timed<T> =
{
	tick:Float,
	val:T,
}

enum Movement
{
	LEFT;
	RIGHT;
	UP;
	DOWN;
}

typedef Control =
{
	movement:Array<Movement>,
	change_weapon:Bool,
	new_weapon_i:Int,
	run:Bool,
	shoot:Bool,
	facing_angle:Float,
}

typedef Weapon =
{
	id:Int,
	name:String,
	spread:Float,
	rate:Float,
	damage:Float,
	range:Int,
	bullet_speed:Int,
	bullet_count:Int,
}

typedef EntityDesc =
{
	id:Int,
	type:Int,
	name:String,
	health:Float,
	max_health:Float,
	x:Float,
	y:Float,
	radius:Float,
	move_speed:Float,
	weapons:haxe.ds.Vector<Int>,
	weapon_i:Int,
	// used for bullet random generation
	random_state:haxe.io.Bytes,
}

typedef Snapshot =
{
	id:Int,
	server_tick:Int,
	client_tick:Int,
	player:EntityDesc,
	entities:haxe.ds.Vector<EntityDesc>,
	despawned:haxe.ds.Vector<Int>,
}

typedef TileSet =
{
	width:Int,
	height:Int,
	tile_size:Int,
	solid:Array<Bool>,
	data:flash.display.BitmapData,
}

typedef TileMap =
{
	width:Int,
	height:Int,
	tile_size:Int,
	scale:Int,
	tiles:Array<Int>,
	flip:Array<Int>,
	collisions:Array<Bool>,
}

typedef WorldConfig =
{
	// client update rate
	client_rate:Int,
	// server update rate
	server_rate:Int,
	// ackSize in bytes
	ack_size:Int,
	// initial snapshot
	snapshot:Snapshot
}

typedef ControlFrame =
{
	client_tick:Int,
	last_snapshot:Int,
	ack:haxe.io.Bytes,
	control:Control,
}

typedef NetworkFrame =
{
	opcode:Int,
	size:Int,
	content:haxe.io.Bytes,
}
