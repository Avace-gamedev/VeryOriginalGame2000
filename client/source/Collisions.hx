import Types.TileMap;

class Collisions
{
	// collision between a ray and a sphere
	public static function raySphereDistance(r_x:Float, r_y:Float, angle:Float, s_x:Float, s_y:Float, radius:Float):Float
	{
		var dir_x = Math.cos(angle);
		var dir_y = Math.sin(angle);

		var o_x = s_x - r_x;
		var o_y = s_y - r_y;

		var t = o_x * dir_x + o_y * dir_y;

		var p_x = r_x + t * dir_x;
		var p_y = r_y + t * dir_y;

		var l_x = s_x - p_x;
		var l_y = s_y - p_y;

		var y = Math.sqrt(l_x * l_x + l_y * l_y);

		if (y > radius)
			return -1;

		var x = Math.sqrt(radius * radius - y * y);

		return t - x;
	}

	public static function rayMapDistance(r_x:Float, r_y:Float, angle:Float, range:Float, map:TileMap):Float
	{
		var pos_x = r_x / map.tile_size;
		var pos_y = r_y / map.tile_size;

		var map_x:Int = Math.floor(pos_x);
		var map_y:Int = Math.floor(pos_y);

		var dir_x = Math.cos(angle);
		var dir_y = Math.sin(angle);

		var delta_dist_x = dir_y == 0 ? 0 : dir_x == 0 ? 1 : Math.abs(1 / dir_x);
		var delta_dist_y = dir_x == 0 ? 0 : dir_y == 0 ? 1 : Math.abs(1 / dir_y);

		var step_x;
		var step_y;
		var side_dist_x;
		var side_dist_y;

		if (dir_x < 0)
		{
			step_x = -1;
			side_dist_x = (pos_x - map_x) * delta_dist_x;
		}
		else
		{
			step_x = 1;
			side_dist_x = (map_x + 1 - pos_x) * delta_dist_x;
		}

		if (dir_y < 0)
		{
			step_y = -1;
			side_dist_y = (pos_y - map_y) * delta_dist_y;
		}
		else
		{
			step_y = 1;
			side_dist_y = (map_y + 1 - pos_y) * delta_dist_y;
		}

		var hit = false;
		var side = 0;
		var wall_dist:Float = 0;
		while (!hit)
		{
			if (side_dist_x < side_dist_y)
			{
				side_dist_x += delta_dist_x;
				map_x += step_x;
				side = 0;
			}
			else
			{
				side_dist_y += delta_dist_y;
				map_y += step_y;
				side = 1;
			}

			hit = map.collisions[map_x + map_y * map.width];

			if (side == 0)
				wall_dist = (map_x - pos_x + (1 - step_x) / 2) / dir_x;
			else
				wall_dist = (map_y - pos_y + (1 - step_y) / 2) / dir_y;

			if (wall_dist > range / map.tile_size)
				return range;
		}

		return wall_dist * map.tile_size;
	}
}
