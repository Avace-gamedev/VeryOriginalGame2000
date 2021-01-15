#pragma once

#include "common/deftypes.h"
#include "common/vector.hpp"
#include "network/network.h"

struct Bullet
{
    ID owner;
    Vec2f pos;
    float angle;
    float damage;
    int range;
};

struct Weapon
{
    WEAPON_ID id;
    std::string name;
    float spread; // in degrees
    float rate;   // in shots/sec
    float damage;
    int range;        // in pixels
    int bullet_speed; // in pixels/sec (0 = hitscan)
    int bullet_count;

    const int write(NetworkFrame &frame) const;
    static const int size();    
};

class Weapons
{
    static std::vector<Weapon> weapons;
public:
    static void loadFromFile(std::string file_path);

    static int nWeapons() { return (int)weapons.size(); };

    static int write(NetworkFrame& frame);
    static int size();

    static bool mem(WEAPON_ID id);
    static bool get(WEAPON_ID id, Weapon *weapon);
};