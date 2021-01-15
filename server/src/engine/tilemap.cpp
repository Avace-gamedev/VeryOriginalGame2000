#include "engine/tilemap.h"

#include <filesystem>
#include <sstream>
#include "loguru/loguru.hpp"
#include "tinyxml/tinyxml2.h"

#include "common/vector.hpp"
#include "common/utils.h"

namespace fs = std::filesystem;

const framesize_t TilesetDesc::write(NetworkFrame &frame) const
{
    framesize_t size = frame.size();

    frame.append(&width, sizeof(width));
    frame.append(&height, sizeof(height));
    frame.append(&tile_size, sizeof(tile_size));

    int written = solid->blitIn(&frame.content()[frame.size()]);
    frame.size() += written;

    frame.append(&data_len, sizeof(data_len));
    frame.append(data, data_len);

    return frame.size() - size;
}

const framesize_t TilesetDesc::size() const
{
    framesize_t size = 0;
    size += sizeof(int);
    size += sizeof(int);
    size += sizeof(uint8_t);
    size += solid->size();
    size += sizeof(framesize_t);
    size += data_len;
    return size;
}

const Vec2i TilemapDesc::xyOfIndex(const int i) const
{
    return Vec2i(i % width, i / width);
}

const Vec2i TilemapDesc::xyOfWorldPos(const Vec2f pos) const
{
    return Vec2i((int)(pos.x / (tile_size * scale)), (int)(pos.y / (tile_size * scale)));
}

const Vec2f TilemapDesc::worldPosOfXY(const Vec2i xy) const
{
    return Vec2f((float)xy.x * tile_size * scale, (float)xy.y * tile_size * scale);
}

int TilemapDesc::indexOfXY(const int x, const int y) const
{
    return y * width + x;
}

const TILE TilemapDesc::getTile(const int x, const int y) const
{
    int address = indexOfXY(x, y);
    if (address >= 0 && address < width * height)
        return tiles[address];
    return -1;
}

const TILE TilemapDesc::getTile(const Vec2i xy) const
{
    return getTile(xy.x, xy.y);
}

void TilemapDesc::setTile(const int x, const int y, const TILE tile)
{
    int address = indexOfXY(x, y);
    if (address >= 0 && address < width * height)
        tiles[address] = tile;
}

void TilemapDesc::setTile(const Vec2i xy, const TILE tile)
{
    setTile(xy.x, xy.y, tile);
}

const bool TilemapDesc::getSolid(const int x, const int y) const
{
    return collisions->get(indexOfXY(x, y));
}

const bool TilemapDesc::getSolid(const Vec2i xy) const
{
    return getSolid(xy.x, xy.y);
}

void TilemapDesc::setBorder(const int n, const TILE tile)
{
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < n; y++)
            setTile(x, y, tile);
        for (int y = height - n; y < height; y++)
            setTile(x, y, tile);
    }

    for (int y = n; y < height - n; y++)
    {
        for (int x = 0; x < n; x++)
            setTile(x, y, tile);
        for (int x = width - n; x < width; x++)
            setTile(x, y, tile);
    }
}

void TilemapDesc::fill(const TILE tile)
{
    for (int i = 0; i < width * height; i++)
        tiles[i] = tile;
}

const framesize_t TilemapDesc::write(NetworkFrame &frame) const
{
    framesize_t size = frame.size();

    frame.append(&width, sizeof(width));
    frame.append(&height, sizeof(height));
    frame.append(&tile_size, sizeof(tile_size));
    frame.append(&scale, sizeof(scale));
    frame.append(tiles, (framesize_t)(width * height * sizeof(TILE)));

    return frame.size() - size;
}

const framesize_t TilemapDesc::size() const
{
    framesize_t size = 0;
    size += sizeof(int);
    size += sizeof(int);
    size += sizeof(uint8_t);
    size += sizeof(uint8_t);
    size += (framesize_t)(width * height * sizeof(TILE));
    return size;
}

TilemapLoader::TilemapLoader(const char *path)
{
    if (fs::exists(path))
        loadMap(path);
    else
        LOG_F(ERROR, "could not load map: %s no such file or directory", path);
}

bool TilemapLoader::loadMap(const char *path)
{
    LOG_F(INFO, "loading %s", path);

    tinyxml2::XMLDocument doc;
    int ret = doc.LoadFile(path);
    if (ret != tinyxml2::XML_SUCCESS)
    {
        LOG_F(ERROR, "map could not be loaded from path %s: %d", path, ret);
        return false;
    }

    tinyxml2::XMLElement *map_element = doc.FirstChildElement("map");

    map.width = map_element->IntAttribute("width");
    map.height = map_element->IntAttribute("height");
    map.tile_size = map_element->IntAttribute("tilewidth");
    if (map_element->IntAttribute("tileheight") != map.tile_size)
        LOG_F(WARNING, "map has different tile width and tile height (%d and %d), ignoring tile height", map.tile_size, map_element->IntAttribute("tileheight"));

    LOG_F(INFO, "map grid dimension %d, %d, tile size %d", map.width, map.height, map.tile_size);

    int flatsize = map.width * map.height;
    map.tiles = (TILE *)malloc(flatsize * sizeof(TILE));
    for (int i = 0; i < flatsize; i++)
        map.tiles[i] = 0;
    map.collisions = new BitArray(flatsize);

    tinyxml2::XMLElement *tileset = map_element->FirstChildElement("tileset");
    const char *tileset_filename = tileset->Attribute("source");
    fs::path this_path(path);
    fs::path tileset_filepath(tileset_filename);
    fs::path tileset_path = this_path.parent_path() / tileset_filepath;
    if (!loadTileset(tileset_path.string().c_str()))
        return false;

    for (tinyxml2::XMLElement *e = map_element->FirstChildElement("layer"); e != NULL; e = e->NextSiblingElement("layer"))
    {
        const char *name = e->Attribute("name");
        LOG_F(INFO, "loading layer %s", name);

        int width = e->IntAttribute("width");
        int height = e->IntAttribute("height");
        const char *data = e->FirstChildElement("data")->GetText();

        std::stringstream s_stream(data);
        for (int i = 0; i < width * height; i++)
        {
            unsigned int tile_id;
            s_stream >> tile_id;
            if (tile_id > 0)
            {
                TILE id = (TILE)tile_id;
                memcpy(&map.tiles[i], &id, sizeof(id));

                int actual_id = (id << 3) >> 3;
                map.collisions->set(i, set.solid->get(actual_id -1));
            }

            if (s_stream.peek() == ',')
                s_stream.ignore();
        }
    }

    return true;
}

bool TilemapLoader::loadTileset(const char *path)
{
    LOG_F(INFO, "loading %s", path);

    tinyxml2::XMLDocument doc;
    int ret = doc.LoadFile(path);
    if (ret != tinyxml2::XML_SUCCESS)
    {
        LOG_F(ERROR, "tileset could not be loaded from path %s: %d", path, ret);
        return false;
    }

    tinyxml2::XMLElement *tileset_element = doc.FirstChildElement("tileset");

    // tile_size
    set.tile_size = tileset_element->IntAttribute("tilewidth");
    if (tileset_element->IntAttribute("tileheight") != set.tile_size)
        LOG_F(WARNING, "tileset has different tile width and tile height (%d and %d), ignoring tile height", set.tile_size, tileset_element->IntAttribute("tileheight"));

    // width and height
    tinyxml2::XMLElement *image = tileset_element->FirstChildElement("image");
    set.width = image->IntAttribute("width") / set.tile_size;
    set.height = image->IntAttribute("height") / set.tile_size;

    LOG_F(INFO, "tileset grid dimension %d, %d, tile size %d", set.width, set.height, set.tile_size);

    // read image, it will be sent to clients
    const char *image_filename = image->Attribute("source");
    fs::path this_path(path);
    fs::path image_filepath(image_filename);
    fs::path image_path = this_path.parent_path() / image_filepath;
    set.data = readFileBytes(image_path.string().c_str(), &set.data_len);

    // collision map
    set.solid = new BitArray(set.width * set.height);

    for (tinyxml2::XMLElement *e = tileset_element->FirstChildElement("tile"); e != NULL; e = e->NextSiblingElement("tile"))
    {
        int i = e->IntAttribute("id");

        tinyxml2::XMLElement *properties = e->FirstChildElement("properties");
        for (tinyxml2::XMLElement *p = properties->FirstChildElement("property"); p != NULL; p = p->NextSiblingElement("property"))
        {
            const char *name = p->Attribute("name");
            const char *type = p->Attribute("type");
            if (name && strcmp(name, "solid") == 0 && type && strcmp(type, "bool") == 0)
                set.solid->set(i, p->BoolAttribute("value"));
        }
    }

    return true;
} 