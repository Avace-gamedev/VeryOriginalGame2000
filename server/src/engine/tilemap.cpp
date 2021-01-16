#include "engine/tilemap.h"

#include <filesystem>
#include <sstream>
#include "loguru/loguru.hpp"

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
    size += (framesize_t)data_len; // we will probably never need more than an int32 to encore the size
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

Map::Map(int scale)
{
    tilemap.scale = scale;
}

bool Map::load(const char *path)
{
    if (!fs::exists(path))
    {
        LOG_F(ERROR, "could not load map: %s no such file or directory", path);
        return false;
    }

    LOG_F(INFO, "loading %s", path);

    tinyxml2::XMLDocument doc;
    int ret = doc.LoadFile(path);
    if (ret != tinyxml2::XML_SUCCESS)
    {
        LOG_F(ERROR, "map could not be loaded from path %s: %d", path, ret);
        return false;
    }

    tinyxml2::XMLElement *map_element = doc.FirstChildElement("map");

    tilemap.width = map_element->IntAttribute("width");
    tilemap.height = map_element->IntAttribute("height");
    tilemap.tile_size = map_element->IntAttribute("tilewidth");
    if (map_element->IntAttribute("tileheight") != tilemap.tile_size)
        LOG_F(WARNING, "map has different tile width and tile height (%d and %d), ignoring tile height", tilemap.tile_size, map_element->IntAttribute("tileheight"));

    LOG_F(INFO, "map grid dimension %d, %d, tile size %d * %hu", tilemap.width, tilemap.height, tilemap.tile_size, tilemap.scale);

    int flatsize = tilemap.width * tilemap.height;
    tilemap.tiles = (TILE *)malloc(flatsize * sizeof(TILE));
    for (int i = 0; i < flatsize; i++)
        tilemap.tiles[i] = 0;
    tilemap.collisions = new BitArray(flatsize);

    tinyxml2::XMLElement *tileset = map_element->FirstChildElement("tileset");
    const char *tileset_filename = tileset->Attribute("source");
    fs::path this_path(path);
    fs::path tileset_filepath(tileset_filename);
    fs::path tileset_path = this_path.parent_path() / tileset_filepath;
    if (!loadTileset(tileset_path.string().c_str()))
        return false;

    for (tinyxml2::XMLElement *e = map_element->FirstChildElement("layer"); e != nullptr; e = e->NextSiblingElement("layer"))
        loadLayer(e);

    for (tinyxml2::XMLElement *e = map_element->FirstChildElement("objectgroup"); e != nullptr; e = e->NextSiblingElement("objectgroup"))
    {
        const char *name = e->Attribute("name");
        if (strcmp(name, "enemies") == 0)
            loadEnemies(e);
        else if (strcmp(name, "spawns") == 0)
            loadSpawns(e);
        else
            LOG_F(WARNING, "unknown object group named %s, ignored", name);
    }

    return true;
}

void Map::loadEnemies(tinyxml2::XMLElement *layer)
{
    Group<Enemy> group;
    group.id = layer->IntAttribute("id");

    LOG_F(INFO, "loading enemy group %d", group.id);

    for (tinyxml2::XMLElement *e = layer->FirstChildElement("object"); e != nullptr; e = e->NextSiblingElement("object"))
    {
        Enemy enemy;
        enemy.name = e->Attribute("name");
        enemy.pos.x = e->FloatAttribute("x") * tilemap.scale;
        enemy.pos.y = e->FloatAttribute("y") * tilemap.scale;

        tinyxml2::XMLElement *properties = e->FirstChildElement("properties");
        for (tinyxml2::XMLElement *e = layer->FirstChildElement("object"); e != nullptr; e = e->NextSiblingElement("object"))
        {
            const char *name = e->Attribute("name");
            const char *type = e->Attribute("type");
            if (strcmp(name, "difficulty") == 0 && strcmp(type, "int") == 0)
                enemy.difficulty = e->IntAttribute("value");
        }

        group.elts.push_back(enemy);
    }
    enemy_groups.push_back(group);
}

void Map::loadSpawns(tinyxml2::XMLElement *layer)
{
    Group<SpawnPoint> group;
    group.id = layer->IntAttribute("id");

    LOG_F(INFO, "loading spawn group %d", group.id);

    for (tinyxml2::XMLElement *e = layer->FirstChildElement("object"); e != nullptr; e = e->NextSiblingElement("object"))
    {
        SpawnPoint spawn_point;
        spawn_point.id = e->IntAttribute("id");
        spawn_point.pos.x = e->FloatAttribute("x") * tilemap.scale;
        spawn_point.pos.y = e->FloatAttribute("y") * tilemap.scale;

        group.elts.push_back(spawn_point);
    }
    spawn_points.push_back(group);
}

void Map::loadLayer(tinyxml2::XMLElement *layer)
{
    const char *name = layer->Attribute("name");
    LOG_F(INFO, "loading layer %s", name);

    int width = layer->IntAttribute("width");
    int height = layer->IntAttribute("height");
    const char *data = layer->FirstChildElement("data")->GetText();

    std::stringstream s_stream(data);
    for (int i = 0; i < width * height; i++)
    {
        unsigned int tile_id;
        s_stream >> tile_id;
        if (tile_id > 0)
        {
            TILE id = (TILE)tile_id;
            memcpy(&tilemap.tiles[i], &id, sizeof(id));

            int actual_id = (id << 3) >> 3;
            tilemap.collisions->set(i, tileset.solid->get(actual_id - 1));
        }

        if (s_stream.peek() == ',')
            s_stream.ignore();
    }
}

bool Map::loadTileset(const char *path)
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
    tileset.tile_size = tileset_element->IntAttribute("tilewidth");
    if (tileset_element->IntAttribute("tileheight") != tileset.tile_size)
        LOG_F(WARNING, "tileset has different tile width and tile height (%d and %d), ignoring tile height", tileset.tile_size, tileset_element->IntAttribute("tileheight"));

    // width and height
    tinyxml2::XMLElement *image = tileset_element->FirstChildElement("image");
    tileset.width = image->IntAttribute("width") / tileset.tile_size;
    tileset.height = image->IntAttribute("height") / tileset.tile_size;

    LOG_F(INFO, "tileset grid dimension %d, %d, tile size %d", tileset.width, tileset.height, tileset.tile_size);

    // read image, it will be sent to clients
    const char *image_filename = image->Attribute("source");
    fs::path this_path(path);
    fs::path image_filepath(image_filename);
    fs::path image_path = this_path.parent_path() / image_filepath;
    tileset.data = readFileBytes(image_path.string().c_str(), &tileset.data_len);

    // collision map
    tileset.solid = new BitArray(tileset.width * tileset.height);

    for (tinyxml2::XMLElement *e = tileset_element->FirstChildElement("tile"); e != nullptr; e = e->NextSiblingElement("tile"))
    {
        int i = e->IntAttribute("id");

        tinyxml2::XMLElement *properties = e->FirstChildElement("properties");
        for (tinyxml2::XMLElement *p = properties->FirstChildElement("property"); p != nullptr; p = p->NextSiblingElement("property"))
        {
            const char *name = p->Attribute("name");
            const char *type = p->Attribute("type");
            if (name && strcmp(name, "solid") == 0 && type && strcmp(type, "bool") == 0)
                tileset.solid->set(i, p->BoolAttribute("value"));
        }
    }

    return true;
}