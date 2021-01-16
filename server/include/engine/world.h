#pragma once

#include <windows.h>
#include <vector>
#include <deque>

#include "common/deftypes.h"
#include "engine/game_config.h"
#include "engine/player.h"

class Map;

// This is the information sent at every server tick to everyone
struct Snapshot
{
    ID id;
    tick_t tick;
    std::vector<EntityDesc> entities;
    std::vector<ID> despawned_entities;

    const int write(NetworkFrame &frame, const Player *player, const TilemapDesc *TilemapDesc = nullptr) const;
    const int size() const;
};

// A snapshot + extra information, this is what the server sends initially
struct WorldConfig
{
    uint8_t client_rate;
    uint8_t server_rate;
    uint8_t ack_size;
    Snapshot *initial_snapshot;

    const int write(NetworkFrame &frame, const Player *player, const TilemapDesc *TilemapDesc = nullptr) const;
    const int size() const;
};

// A control + extra information, this is what the server receives
struct ControlFrame
{
    tick_t reception_server_tick;

    // read fr
    ID player_id;
    ID last_snapshot;
    char ack[ACK_SIZE];
    Control control;

    static ControlFrame read(NetworkFrame frame);
};

struct SpawnPoint
{
    ID id;
    Vec2f pos;
};

class World
{
    ID snapshot_id = 0;
    std::deque<Snapshot> snapshot_history;

    Map *map;

    std::vector<Player *> players;
    std::vector<Entity *> entities;
    std::vector<ID> dropped_players;

    int getPlayerIndexById(const ID id) const;
    const Snapshot *getSnapshotAtTick(tick_t tick) const;

public:
    World(Map *map);
    ~World();

    Player *createPlayer(ID id, sockaddr_in from, std::string name);
    void dropPlayer(const ID id);

    void add(Entity *entity);
    void add(Player *entity);

    void update(tick_t current_tick);
    void doHitScan(Bullet bullet, tick_t tick);

    const int getNPlayers() const;
    const std::vector<Player *> &getPlayers() const;
    const int getNEntities() const;

    Snapshot makeSnapshot(tick_t current_tick);
    WorldConfig makeConfig(Snapshot *snapshot);

    void remember(Snapshot snapshot);

    Entity *getById(const ID id, bool *is_player = nullptr) const;
    Player *getPlayerById(const ID id) const;
};
