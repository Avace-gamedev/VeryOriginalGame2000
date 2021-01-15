#pragma once

#include <stdint.h>

#include "common/vector.hpp"
#include "common/bitarray.h"
#include "network/network.h"

typedef uint32_t TILE;

struct TilesetDesc
{
    int width;
    int height;
    uint8_t tile_size;
    BitArray *solid;
    int data_len;
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

class TilemapLoader
{
    TilemapDesc map;
    TilesetDesc set;

    int tileset_count;

    bool loadMap(const char *path);
    bool loadTileset(const char *path);

public:
    TilemapLoader(const char *path);
    TilemapDesc getDesc() { return map; };
    TilesetDesc getSet() { return set; };
};