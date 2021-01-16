import Types.TileMap;
import Types.TileSet;
import flixel.FlxSprite;
import flixel.system.FlxAssets.FlxShader;
import openfl.display.BitmapData;

class MapShader extends FlxShader
{
	@:glFragmentSource('
    #pragma header

    uniform sampler2D tileset;
    uniform vec2 tileset_size;

    uniform sampler2D tilemap;
    uniform vec2 tilemap_size;
    
    uniform float tile_size;

    void main() {
        vec2 grid_pos_float = openfl_TextureCoordv * tilemap_size;
        vec2 grid_pos_int = floor(grid_pos_float);
        
        vec4 aux = flixel_texture2D(tilemap, (grid_pos_int + 0.5) / tilemap_size);

        int tile_id = int(aux.z * 0xFF);
        vec2 tile_ij = vec2(mod(tile_id, tileset_size.x) - 1, int(tile_id / tileset_size.x));
        vec2 tile_xy = tile_ij / tileset_size;

        int tile_flip = int(aux.y * 0xFF);
        bool flip_hori = ((tile_flip >> 2) % 2 == 1);
        bool flip_vert = ((tile_flip >> 1) % 2 == 1);
        bool flip_diag = ((tile_flip     ) % 2 == 1);

        vec2 cell_pos = grid_pos_float - grid_pos_int;
        if (flip_hori)
            cell_pos = vec2(1 - cell_pos.x, cell_pos.y);
        if (flip_vert)
            cell_pos = vec2(cell_pos.x, 1 - cell_pos.y);
        if (flip_diag)
            cell_pos.xy = cell_pos.yx;

        vec2 tileset_pos = tile_ij + cell_pos;

        vec4 col = flixel_texture2D(tileset, tileset_pos / tileset_size);

        gl_FragColor = col;
    }
    ')
	override public function new(tilemap:TileMap, tileset:TileSet, bw:Bool = true)
	{
		super();

		if (!bw)
		{
			data.tileset.input = tileset.data;
			data.tileset_size.value = [tileset.width, tileset.height];

			// tile id is stored in blue channel, rotation in green
			var tilemap_bmp = new BitmapData(tilemap.width, tilemap.height, 0xFF000000);
			for (x in 0...tilemap.width)
				for (y in 0...tilemap.height)
				{
					var pixel = 0xFFFF0000;
					pixel += tilemap.flip[x + y * tilemap.width] << 8; // tile flip, 3 bits
					pixel += tilemap.tiles[x + y * tilemap.width]; // tile id, 1 byte

					tilemap_bmp.setPixel(x, y, pixel);
				}
			data.tilemap.input = tilemap_bmp;
			data.tilemap_size.value = [tilemap.width, tilemap.height];
		}
		else
		{
			// no graphics, walls are black, other tiles are white
			var tileset_bmp = new BitmapData(2 * tilemap.tile_size, tilemap.tile_size);
			for (i in 0...tilemap.tile_size)
				for (j in 0...tilemap.tile_size)
					tileset_bmp.setPixel(tilemap.tile_size + i, j, 0);
			data.tileset.input = tileset_bmp;
			data.tileset_size.value = [2, 1];

			var tilemap_bmp = new BitmapData(tilemap.width, tilemap.height, 0xFF000000);
			for (x in 0...tilemap.width)
				for (y in 0...tilemap.height)
					tilemap_bmp.setPixel(x, y, 0xFF000000 + (tileset.solid[x + y * tilemap.width] ? 1 : 0));
			data.tilemap.input = tilemap_bmp;
			data.tilemap_size.value = [tilemap.width, tilemap.height];
			data.tile_size.value = [tilemap.tile_size];
		}
	}
}

class LightShader extends FlxShader
{
	@:glFragmentSource('
    #pragma header

    #define PI 3.1415926538
    #define E  2.7182818285

    uniform sampler2D tilemap;
    uniform vec2 tilemap_size;
    uniform vec2 light_pos;
    uniform float sight_radius;

    vec2 tile_size = 1 / tilemap_size; // relative tile size

    bool isSolid(vec2 tile)
    {
        vec4 tex = flixel_texture2D(tilemap, tile);
        return tex.z > 0;
    }

    float rayMapDistance(vec2 ray_origin, float angle, float range)
    {
        vec2 pos = ray_origin / tile_size;         // integer = cell of grid, floating part = position in cell
        vec2 map = floor(pos);                     // cell of grid
        vec2 dir = vec2(cos(angle), sin(angle));

        vec2 delta_dist = vec2(dir.y == 0 ? 0 : dir.x == 0 ? 1 : abs(1 / dir.x), 
                               dir.x == 0 ? 0 : dir.y == 0 ? 1 : abs(1 / dir.y));
        vec2 step;
        vec2 side_dist;

        if (dir.x < 0)
        {
            step.x = -1;
            side_dist.x = (pos.x - map.x) * delta_dist.x;
        }
        else 
        {
            step.x = 1;
            side_dist.x = (map.x + 1 - pos.x) * delta_dist.x;
        }

        if (dir.y < 0)
        {
            step.y = -1;
            side_dist.y = (pos.y - map.y) * delta_dist.y;
        }
        else 
        {
            step.y = 1;
            side_dist.y = (map.y + 1 - pos.y) * delta_dist.y;
        }

        bool hit = false;
        int side = 0;
        float wall_dist = 0;
        while (!hit)
        {
            if (side_dist.x < side_dist.y)
            {
                side_dist.x += delta_dist.x;
                map.x += step.x;
                side = 0;
            }
            else 
            {
                side_dist.y += delta_dist.y;
                map.y += step.y;
                side = 1;
            }

            hit = isSolid((map+1) / tilemap_size);

            if (side == 0)
                wall_dist = (map.x - pos.x + (1 - step.x) / 2) / dir.x / tilemap_size;
            else
                wall_dist = (map.y - pos.y + (1 - step.y) / 2) / dir.y / tilemap_size;

            if (wall_dist > range)
                return range;
        }

        return wall_dist;
    }

    float AO(vec2 p, float dist, float radius, float intensity)
    {
        float a = clamp(dist / radius, 0.0, 1.0) - 1.0;
        return 1.0 - (pow(abs(a), 5.0) + 1.0) * intensity + (1.0 - intensity);
    }

    bool inShadow(vec2 pos) {
        vec2 normalized_light_pos = light_pos / openfl_TextureSize;
        float light_frag_angle = atan(normalized_light_pos.y - pos.y, normalized_light_pos.x - pos.x);
        float light_frag_dist = length(normalized_light_pos - pos);
        float collision_dist = rayMapDistance(pos, light_frag_angle, light_frag_dist);

        return isSolid(pos) || collision_dist < light_frag_dist;
    }

    void main() {       
        
        vec2 normalized_light_pos = light_pos / openfl_TextureSize;
        float light_frag_angle = atan(normalized_light_pos.y - openfl_TextureCoordv.y, normalized_light_pos.x - openfl_TextureCoordv.x);
        float light_frag_dist = length(normalized_light_pos - openfl_TextureCoordv);
        float collision_dist = rayMapDistance(openfl_TextureCoordv, light_frag_angle, light_frag_dist);

        vec4 col = vec4(1,1,1,1);
        
        vec4 walls;
        if (isSolid(openfl_TextureCoordv))
            walls = vec4(0,0,0,0);
        else 
            walls = vec4(1,1,1,1);

        vec4 ambient; 
        ambient = AO(openfl_TextureCoordv, collision_dist, 40.0, 0.4);

        vec2 sight = sight_radius / tilemap_size; // in number of tiles

        vec4 sight_dist_shadow;
        float radius_along_angle = 
            sqrt((cos(light_frag_angle) * sight.x) * (cos(light_frag_angle) * sight.x) + 
                 (sin(light_frag_angle) * sight.x) * (sin(light_frag_angle) * sight.x));
        float fall = 1 - smoothstep(0, radius_along_angle, light_frag_dist);
        fall = clamp(fall, 0, 1);
        sight_dist_shadow = vec4(fall, fall, fall, 1);

        vec4 los_shadow;
        if (inShadow(openfl_TextureCoordv))
            los_shadow = vec4(0, 0, 0, 1);
        else
            los_shadow = vec4(1, 1, 1, 1);

        vec4 shadow = clamp(sight_dist_shadow * los_shadow, 0.5, 1);

        col *= shadow;
        col *= walls;
        // col *= ambient;

        gl_FragColor = vec4(col.xyz, 1);
    }')
	override public function new(tilemap:TileMap, tileset:TileSet, sight_radius:Float = 25)
	{
		super();

		// tile value is stored in blue channel
		var bmp = new BitmapData(tilemap.width, tilemap.height, 0xFF000000);
		for (x in 0...tilemap.width)
			for (y in 0...tilemap.height)
				bmp.setPixel(x, y, 0xFF000000 + (tileset.solid[tilemap.tiles[x + y * tilemap.width] - 1] ? 0xFF : 0x00));

		data.tilemap.input = bmp;
		data.tilemap_size.value = [tilemap.width, tilemap.height];
		data.sight_radius.value = [sight_radius];
	}

	public function setLightPos(x:Float, y:Float)
	{
		data.light_pos.value = [x, y];
	}
}
