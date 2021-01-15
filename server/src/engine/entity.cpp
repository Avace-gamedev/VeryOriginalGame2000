#include "engine/entity.h"

#include <vector>
#include "loguru/loguru.hpp"

#include "common/utils.h"
#include "network/network.h"
#include "engine/game_config.h"
#include "engine/controller.h"

#define M_PI 3.14159265359f

const framesize_t EntityDesc::write(NetworkFrame &frame, bool player) const
{
    framesize_t size = frame.size();

    frame.append(&id, sizeof(id));

    if (!player)
        frame.append(&type, sizeof(type));

    frame.appendString(name);
    frame.append(&health, sizeof(health));
    frame.append(&max_health, sizeof(max_health));
    frame.append(&x, sizeof(x));
    frame.append(&y, sizeof(y));
    frame.append(&radius, sizeof(radius));

    if (player)
        frame.append(&velocity, sizeof(velocity));

    WEAPON_ID no_weapon = -1;
    for (int i = 0; i < MAX_N_WEAPONS; i++)
        if (i < weapons.size())
            frame.append(&weapons[i], sizeof(weapons[i]));
        else
            frame.append(&no_weapon, sizeof(no_weapon));

    frame.append(&weapon_i, sizeof(weapon_i));
    frame.append(&random_state, XORSHIFT64PLUS_STATE_SIZE);

    return frame.size() - size;
}

const framesize_t EntityDesc::size()
{
    framesize_t size = 0;
    size += sizeof(ID);
    size += sizeof(EntityType);
    size += NAME_SIZE;
    size += sizeof(int);
    size += sizeof(int);
    size += sizeof(float);
    size += sizeof(float);
    size += sizeof(float);
    size += sizeof(float);
    size += sizeof(float);
    size += XORSHIFT64PLUS_STATE_SIZE;
    size += sizeof(WEAPON_ID) * MAX_N_WEAPONS;
    size += sizeof(int);
    return size;
}

Entity::Entity(ID id = -1, EntityType type, std::string name, float max_health, Controller *controller) : type(type), max_health(max_health), health(max_health), controller(controller)
{
    if (id < 0)
        this->id = freshID();
    else
        this->id = id;

    setName(name);

    if (!Weapons::get(0, &equipped_weapon))
        LOG_F(ERROR, "no weapon with ID 0");

    pickWeapon(0);
}

Entity::~Entity()
{
    if (controller)
        delete controller;
}

void Entity::setName(const std::string name)
{
    if (name.size() <= NAME_SIZE)
        this->name = name;
}

void Entity::hurt(const float damage)
{
    health -= damage;
    if (health <= 0)
    {
        health = 0;
        kill();
    }
}

void Entity::heal(const float health) { hurt(-health); }

bool Entity::isAlive() { return alive; }

void Entity::place(const Vec2f new_pos)
{
    last_pos = new_pos;
    pos = new_pos;
}

void Entity::face(const float angle) { facing_angle = angle; }

void Entity::move(const Vec2f dpos)
{
    last_pos = pos;
    pos += dpos;
}

void Entity::kill() { alive = false; }

EntityDesc Entity::getSnapshot()
{
    EntityDesc snapshot;
    snapshot.id = id;
    snapshot.type = type;
    snapshot.name = name;
    snapshot.health = health;
    snapshot.max_health = max_health;
    snapshot.x = pos.x;
    snapshot.y = pos.y;
    snapshot.radius = radius;
    snapshot.facing_angle = facing_angle;
    snapshot.velocity = max_velocity;

    for (int i = 0; i < weapons.size(); i++)
        snapshot.weapons.push_back(weapons[i]);
    snapshot.weapon_i = weapon_i;
    random_generator.writeState(snapshot.random_state);

    return snapshot;
}

const Vec2f Entity::middle() const
{
    return Vec2f(pos.x + radius, pos.y + radius);
}

const Vec2f Entity::top() const
{
    return Vec2f(pos.x + radius, pos.y);
}

const Vec2f Entity::bottom() const
{
    return Vec2f(pos.x + radius, pos.y + 2 * radius);
}

const Vec2f Entity::left() const
{
    return Vec2f(pos.x, pos.y + radius);
}

const Vec2f Entity::right() const
{
    return Vec2f(pos.x + 2 * radius, pos.y + radius);
}

const Vec2f Entity::topleft() const
{
    return Vec2f(pos.x, pos.y);
}

const Vec2f Entity::bottomleft() const
{
    return Vec2f(pos.x, pos.y + 2 * radius);
}

const Vec2f Entity::topright() const
{
    return Vec2f(pos.x + 2 * radius, pos.y);
}

const Vec2f Entity::bottomright() const
{
    return Vec2f(pos.x + 2 * radius, pos.y + 2 * radius);
}

void Entity::doMapCollisions(const TilemapDesc *map)
{
    int tile_size = map->tile_size * map->scale;
    Vec2i tile_topleft = map->xyOfWorldPos(topleft());
    if (map->getSolid(tile_topleft))
    {
        Vec2f pos_tile = map->worldPosOfXY(tile_topleft);
        float overlapX = computeOverlapX(pos, last_pos, Vec2f(2 * radius, 2 * radius), pos_tile, pos_tile, Vec2f((float)tile_size, (float)tile_size));
        pos.x -= overlapX;
        float overlapY = computeOverlapY(pos, last_pos, Vec2f(2 * radius, 2 * radius), pos_tile, pos_tile, Vec2f((float)tile_size, (float)tile_size));
        pos.y -= overlapY;
    }

    Vec2i tile_bottomright = map->xyOfWorldPos(bottomright());
    if (map->getSolid(tile_bottomright))
    {
        Vec2f pos_tile = map->worldPosOfXY(tile_bottomright);
        float overlapX = computeOverlapX(pos, last_pos, Vec2f(2 * radius, 2 * radius), pos_tile, pos_tile, Vec2f((float)tile_size, (float)tile_size));
        pos.x -= overlapX;
        float overlapY = computeOverlapY(pos, last_pos, Vec2f(2 * radius, 2 * radius), pos_tile, pos_tile, Vec2f((float)tile_size, (float)tile_size));
        pos.y -= overlapY;
    }
}

void Entity::applyControl(const Control *ctrl)
{
    running = ctrl->run;

    bool up = UP(ctrl->movement);
    bool down = DOWN(ctrl->movement);
    bool left = LEFT(ctrl->movement);
    bool right = RIGHT(ctrl->movement);

    float aux_x = (left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f);
    float aux_y = (up ? -1.0f : 0.0f) + (down ? 1.0f : 0.0f);
    float aux_norm = (float)sqrt((double)aux_x * (double)aux_x + (double)aux_y * (double)aux_y);

    float vel = running ? max_velocity : max_velocity * WALK_SPEED_MOD;

    if (aux_norm > 0)
        move(Vec2f(aux_x * vel / aux_norm, aux_y * vel / aux_norm));

    if (ctrl->change_weapon)
    {
        weapon_i = ctrl->new_weapon_i;
        Weapons::get(weapon_i, &equipped_weapon);
        tick_next_shoot = ctrl->tick + Time::msToTicks(CHWEAP_DELAY, CLIENT_PERIOD);
    }

    want_shoot = ctrl->shoot;
    facing_angle = ctrl->facing_angle;
}

void Entity::pickWeapon(WEAPON_ID id)
{
    if (weapons.size() < MAX_N_WEAPONS)
    {
        weapon_i = (int)weapons.size();
        weapons.push_back(id);
    }
    else
    {
        weapons[weapon_i] = id;
    }
    Weapons::get(id, &equipped_weapon);
}

const std::vector<Control *> Entity::update(const tick_t current_tick, const TilemapDesc *map)
{
    if (!controller)
        return std::vector<Control *>();

    controller->update(this, current_tick);
    std::vector<Control *> ctrls = controller->getControls(current_tick);

    for (Control *ctrl : ctrls)
    {
        applyControl(ctrl);
        doMapCollisions(map);
    }

    return ctrls;
}

bool Entity::canShoot(tick_t current_tick)
{
    return current_tick > tick_next_shoot;
}

void Entity::configBullet(Bullet *bullet)
{
    Vec2f pos = middle();
    float random_noise = (float)random_generator.uniform(-equipped_weapon.spread / 180.0f * M_PI / 2.0f, equipped_weapon.spread / 180.0f * M_PI / 2.0f);

    bullet->owner = id;
    bullet->pos = pos;
    bullet->angle = facing_angle + random_noise;
    bullet->damage = equipped_weapon.damage;
    bullet->range = equipped_weapon.range;
}

void Entity::registerShoot(tick_t current_tick)
{
    want_shoot = false;
    tick_next_shoot = current_tick + Time::msToTicks((unsigned long long)(1000.0f / equipped_weapon.rate), CLIENT_PERIOD);
}