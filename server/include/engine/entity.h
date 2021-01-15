#pragma once

#include <string>
#include <vector>
#include <random>

#include "common/deftypes.h"
#include "common/xorshift64plus.h"
#include "engine/collision.h"
#include "engine/controller.h"
#include "engine/weapon.h"

enum EntityType : uint8_t
{
    ENTITY = 0,
    CONTROLLED_ENTITY = 1,
    PLAYER = 2,
};

struct EntityDesc
{
    ID id;
    EntityType type;
    std::string name;
    float health;
    float max_health;
    float x;
    float y;
    float radius;
    float velocity;
    float facing_angle;
    std::vector<WEAPON_ID> weapons;
    int weapon_i; // index of equipped weapon amongst all weapons
    char random_state[XORSHIFT64PLUS_STATE_SIZE];

    const framesize_t write(NetworkFrame &frame, bool player = false) const;
    static const framesize_t size();    
};

class Entity
{
protected:

    Random random_generator;
    Controller *controller;

    tick_t tick_next_shoot = 0;

    virtual void doMapCollisions(const TilemapDesc *map);
    virtual void applyControl(const Control *control);

public:
    ID id;
    std::string name;
    EntityType type;
    float radius = 9;
    float facing_angle = 0;
    Vec2f pos;
    Vec2f last_pos;
    float max_velocity = 5;
    bool running = false;
    bool alive = true;
    float health;
    float max_health = 100;
    bool want_shoot;

    std::vector<WEAPON_ID> weapons;
    int weapon_i;
    Weapon equipped_weapon;

    Entity(ID id, EntityType type = ENTITY, std::string name = "__entity__", float max_health = 100, Controller *controller = nullptr);
    ~Entity();

    virtual const std::vector<Control *> update(const tick_t current_tick, const TilemapDesc *map);

    bool isAlive();
    void hurt(const float damage);
    void heal(const float health);
    void place(const Vec2f pos);
    void face(const float angle);
    void move(const Vec2f dpos);
    void pickWeapon(WEAPON_ID id);
    void registerShoot(tick_t current_tick);
    void kill();

    void configBullet(Bullet *bullet);
    void setName(std::string name);

    bool canShoot(tick_t current_tick);
    float getHealth() { return health; }
    float getFacingAngle() { return facing_angle; }
    float getSize() { return radius; };

    const Vec2f middle() const;
    const Vec2f top() const;
    const Vec2f bottom() const;
    const Vec2f left() const;
    const Vec2f right() const;
    const Vec2f topleft() const;
    const Vec2f bottomleft() const;
    const Vec2f topright() const;
    const Vec2f bottomright() const;

    virtual EntityDesc getSnapshot();
};