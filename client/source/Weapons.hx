import Types.Weapon;

class Weapons
{
	public static var weapons:haxe.ds.Vector<Weapon>;

	public static function mem(id:Int)
	{
		return id < weapons.length;
	}

	public static function get(id:Int)
	{
		if (mem(id))
			return weapons[id];
		return null;
	}

	public static function toString()
	{
		var name_list = [];
		for (weap in weapons)
			name_list.push(weap.name);

		var names = name_list.join(', ');
		return '[Weapons count:' + weapons.length + ' names:$names]';
	}
}
