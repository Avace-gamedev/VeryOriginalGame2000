#include "engine/world.h"

#include <cstring>
#include <math.h>
#include "loguru/loguru.hpp"

const int Snapshot::write(NetworkFrame &frame, const Player *player, const TilemapDesc *TilemapDesc) const
{
    int size = frame.size();

    frame.append(&id, sizeof(id));
    frame.append(&tick, sizeof(tick));

    tick_t client_tick = player == nullptr ? -1 : player->client_tick;
    frame.append(&client_tick, sizeof(client_tick));

    // EntityDesc::write will update size

    int n_entities = (int)entities.size();

    EntityDesc *player_desc = nullptr;
    for (EntityDesc desc : entities)
        if (player && desc.id == player->id)
        {
            desc.write(frame, true);
            player_desc = &desc;
            break;
        }

    if (!player_desc)
    {
        // player is dead
        ID id = -1;
        frame.append(&id, sizeof(id));
    }

    int n = 0; // number of written entities
    char *n_addr = &frame.content()[frame.size()];
    frame.append(&n, sizeof(n)); // will be updated after the loop

    // EntityDesc::write will update size
    for (EntityDesc desc : entities)
    {
        if (player_desc && TilemapDesc)
        {
            // don't send entity if it is not visible by player
            float player_entity_dist = sqrt((player_desc->x - desc.x) * (player_desc->x - desc.x) + (player_desc->y - desc.y) * (player_desc->y - desc.y));
            float player_entity_angle = atan2(desc.y - player_desc->y, desc.x - player_desc->x);
            float ray_dist = computeDistance(Vec2f(player_desc->x + player_desc->radius, player_desc->y + player_desc->radius), player_entity_angle, player_entity_dist, TilemapDesc);

            if (ray_dist < player_entity_dist)
                continue;
        }

        if (desc.id != player->id)
        {
            desc.write(frame, false);
            n++;
        }
    }

    memcpy(n_addr, &n, sizeof(n));

    int n_despawned = (int)despawned_entities.size();
    frame.append(&n_despawned, sizeof(n_despawned));

    for (ID id : despawned_entities)
    {
        frame.append(&id, sizeof(id));
    }

    frame.opcode() = OP_SNAPSHOT;

    return frame.size() - size;
}

const int Snapshot::size() const
{
    return 16 + EntityDesc::size() * (int)entities.size() + 4 * (int)despawned_entities.size();
}

const int WorldConfig::write(NetworkFrame &frame, const Player *player, const TilemapDesc *TilemapDesc) const
{
    int size = frame.size();

    frame.append(&client_rate, sizeof(client_rate));
    frame.append(&server_rate, sizeof(server_rate));
    frame.append(&ack_size, sizeof(ack_size));

    initial_snapshot->write(frame, player, TilemapDesc);

    frame.opcode() = OP_CONFIG;

    return frame.size() - size;
}

const int WorldConfig::size() const
{
    int size = 0;
    size += sizeof(uint8_t);
    size += sizeof(uint8_t);
    size += sizeof(uint8_t);
    size += initial_snapshot->size();
    return size;
}

ControlFrame ControlFrame::read(NetworkFrame frame)
{
    ControlFrame ctrl_frame;
    ctrl_frame.player_id = frame.sender;

    ctrl_frame.reception_server_tick = Time::nowInTicks(CLIENT_PERIOD);

    const char *buffer = frame.content();

    int size = 0;

    // next 4 bytes contain client tick number
    memcpy(&ctrl_frame.control.tick, &buffer[size], sizeof(tick_t));
    size += sizeof(tick_t);

    // next 4 bytes contain last server tick received
    memcpy(&ctrl_frame.last_snapshot, &buffer[size], sizeof(ID));
    size += sizeof(ID);

    // next ACK_SIZE bytes contain ack from client
    memcpy(&ctrl_frame.ack, &buffer[size], ACK_SIZE);
    size += ACK_SIZE;

    // next byte encodes movement + shoot command
    uint8_t first_byte;
    memcpy(&first_byte, &buffer[size], 1);
    size += 1;

    ctrl_frame.control.movement = first_byte >> 4;
    ctrl_frame.control.change_weapon = (first_byte >> 2 & 3) > 0;
    ctrl_frame.control.new_weapon_i = (first_byte >> 2 & 3) - 1;
    ctrl_frame.control.run = (first_byte >> 1) % 2 == 1 ? true : false;
    ctrl_frame.control.shoot = (first_byte % 2 == 1) ? true : false;

    // next 4 bytes encode facing_angle as a float
    memcpy(&ctrl_frame.control.facing_angle, &buffer[size], 4);

    return ctrl_frame;
}

//
//
//
// class WORLD
//
//
//

World::World(TilemapDesc *map) : map(map)
{
    addSpawn(Vec2f(200, 200));
}

World::~World()
{
    for (Player *player : players)
        delete player;
    for (Entity *entity : entities)
        delete entity;
}

void World::addSpawn(const Vec2f pos)
{
    spawn_point.push_back(pos);
}

Player *World::createPlayer(ID id, sockaddr_in from, std::string name)
{
    Player *player = new Player(id, from, name);
    player->place(spawn_point[0]);
    player->pickWeapon(3);
    add(player);
    return player;
}

void World::dropPlayer(const ID id)
{
    int i = getPlayerIndexById(id);

    if (i < 0)
        return;

    Player *player = players[i];
    player->kill();
    // BAAAM
    dropped_players.push_back(player->id);
    // you'll be barely remembered ...
    players.erase(players.begin() + i);
    // BARELY !!!
    delete player;
    // ARE YOU DEAD YET ?
}

void World::add(Entity *entity) { entities.push_back(entity); }
void World::add(Player *player) { players.push_back(player); }

const int World::getNPlayers() const { return (int)players.size(); }

const std::vector<Player *> &World::getPlayers() const { return players; }

const int World::getNEntities() const { return (int)entities.size(); }

int World::getPlayerIndexById(const ID id) const
{
    for (int i = 0; i < getNPlayers(); i++)
        if (players[i]->id == id)
            return i;
    return -1;
}

Player *World::getPlayerById(const ID id) const
{
    int i = getPlayerIndexById(id);

    if (i < 0)
        return nullptr;

    return players[i];
}

Entity *World::getById(const ID id, bool *is_player) const
{
    Player *player = getPlayerById(id);

    if (player)
    {
        if (is_player)
            *is_player = true;
        return player;
    }

    for (Entity *entity : entities)
        if (entity->id == id)
            return entity;

    return nullptr;
}

void World::update(tick_t current_tick)
{
    for (Player *player : players)
        if (player->want_shoot && player->canShoot(current_tick))
        {
            for (int i = 0; i < player->equipped_weapon.bullet_count; i++) {
                Bullet bullet;
                player->configBullet(&bullet);
                doHitScan(bullet, player->client_tick);
            }
            player->registerShoot(current_tick);
        }

    for (Player *entity : players)
        entity->update(current_tick, map);
    for (Entity *entity : entities)
        entity->update(current_tick, map);
}

void World::doHitScan(Bullet bullet, tick_t tick)
{
    const Snapshot *snapshot = getSnapshotAtTick(tick);
    if (snapshot == nullptr)
    {
        LOG_F(ERROR, "could not find snapshot at tick %d", tick);
        return;
    }

    float closest_dist = -1;
    const EntityDesc* closest_entity = nullptr;

    for (int i = 0; i < snapshot->entities.size(); i++)
    {
        const EntityDesc* entity = &snapshot->entities[i];
        if (entity->id == bullet.owner)
            continue;

        float dist = computeDistance(bullet.pos, bullet.angle, Vec2f(entity->x + entity->radius, entity->y + entity->radius), entity->radius);

        if (dist > 0 && dist <= bullet.range)
        {
            if (closest_dist < 0 || dist < closest_dist)
            {
                closest_dist = dist;
                closest_entity = entity;
            }
        }
    }

    float wall_dist = computeDistance(bullet.pos, bullet.angle, (float)bullet.range, map);

    if (closest_dist < 0 || wall_dist < closest_dist)
        return;

    bool is_player = false;
    Entity *target = getById(closest_entity->id, &is_player);
    if (target) {
        target->hurt(bullet.damage);
    }
}

const Snapshot *World::getSnapshotAtTick(tick_t tick) const
{
    for (int i = 0; i < snapshot_history.size(); i++)
    {
        if (snapshot_history[i].tick <= tick)
            return &snapshot_history[i];
    }
    return nullptr;
}

Snapshot World::makeSnapshot(tick_t current_tick)
{
    Snapshot res;
    if (snapshot_history.size() > 0 && snapshot_history.front().tick == current_tick)
    {
        res = snapshot_history.front();
    }
    else
    {
        res.id = snapshot_id++;
        res.tick = current_tick;

        for (Player *player : players)
            res.entities.push_back(player->getSnapshot());

        for (Entity *entity : entities)
            res.entities.push_back(entity->getSnapshot());

        for (int i = 0; i < dropped_players.size(); i++)
            res.despawned_entities.push_back(dropped_players[i]);
        dropped_players.clear();
    }

    return res;
}

void World::remember(Snapshot snapshot)
{
    snapshot_history.push_front(snapshot);

    while (snapshot_history.back().tick < Time::nowInTicks(CLIENT_PERIOD) - MAX_PING / CLIENT_PERIOD)
        snapshot_history.pop_back();
}