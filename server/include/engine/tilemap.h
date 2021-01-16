#pragma once

#include <stdint.h>
#include "tinyxml/tinyxml2.h"

#include "common/vector.hpp"
#include "common/bitarray.h"
#include "network/network.h"
#include "engine/enemy.h"
#include "engine/world.h"

typedef uint32_t TILE;

struct TilesetDesc
{
    int width;
    int height;
    uint8_t tile_size;
    BitArray *solid;
    size_t data_len;
    char *data; // actual bitmap data

    const framesize_t write(NetworkFrame &frame) const;
    const framesize_t size() const;
};

struct TilemapDesc
{
    int width;
    int height;
    uint8_t tile_size;
    uint8_t scale;
    TILE *tiles;
    BitArray *collisions;

    const Vec2i xyOfIndex(const int i) const;
    int indexOfXY(const int x, const int y) const;

    const Vec2i xyOfWorldPos(const Vec2f pos) const;
    const Vec2f worldPosOfXY(const Vec2i xy) const;

    const TILE getTile(const int x, const int y) const;
    void setTile(const int x, const int y, const TILE tile);

    const TILE getTile(const Vec2i xy) const;
    void setTile(const Vec2i xy, const TILE tile);

    const bool getSolid(const int x, const int y) const;
    const bool getSolid(const Vec2i xy) const;

    void fill(const TILE tile);
    // set n first-last lines-cols
    void setBorder(const int n, const TILE tile);

    const framesize_t write(NetworkFrame &frame) const;
    const framesize_t size() const;
};

template <typename T>
struct Group
{
    ID id;
    std::vector<T> elts;
};

struct Enemy;
struct SpawnPoint;

class Map
{
    int tileset_count;

    TilemapDesc tilemap;
    TilesetDesc tileset;

    void loadLayer(tinyxml2::XMLElement *layer);
    void loadEnemies(tinyxml2::XMLElement *layer);
    void loadSpawns(tinyxml2::XMLElement *layer);
    bool loadTileset(const char *path);

public:
    std::vector<Group<Enemy>> enemy_groups;
    std::vector<Group<SpawnPoint>> spawn_points;

    Map(int scale);

    const TilemapDesc *getTilemap() { return &tilemap; }
    const TilesetDesc *getTileset() { return &tileset; }

    bool load(const char *path);
};