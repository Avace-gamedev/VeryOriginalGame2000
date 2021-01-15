#include <string>

#include "tinyxml/tinyxml2.h"
#include "loguru/loguru.hpp"

#include "network/network.h"
#include "engine/game_config.h"
#include "engine/weapon.h"

using namespace tinyxml2;

const int Weapon::size()
{
	int size = 0;
	size += sizeof(WEAPON_ID);
	size += NAME_SIZE;
	size += sizeof(float);
	size += sizeof(float);
	size += sizeof(float);
	size += sizeof(int);
	size += sizeof(int);
	size += sizeof(int);
	return size;   
}

const int Weapon::write(NetworkFrame &frame) const
{
    int size = frame.size();

    frame.append(&id, sizeof(id));
    frame.appendString(name);
    frame.append(&spread, sizeof(spread));
    frame.append(&rate, sizeof(rate));
    frame.append(&damage, sizeof(damage));
    frame.append(&range, sizeof(range));
    frame.append(&bullet_speed, sizeof(bullet_speed));
    frame.append(&bullet_count, sizeof(bullet_count));

    return frame.size() - size;
}

std::vector<Weapon> Weapons::weapons;

void Weapons::loadFromFile(std::string file_path)
{
	tinyxml2::XMLDocument doc;
	int ret = doc.LoadFile(file_path.c_str());
	if (ret != tinyxml2::XML_SUCCESS)
	{
		LOG_F(ERROR, "weapons couldn't be loaded: %d", ret);
		return;
	}

	tinyxml2::XMLElement *weapons_element = doc.FirstChildElement("weapons");

	for (tinyxml2::XMLElement *e = weapons_element->FirstChildElement("weapon"); e != NULL; e = e->NextSiblingElement("weapon"))
	{
		Weapon weapon;
		weapon.id = (WEAPON_ID)weapons.size();
		weapon.name = std::string(e->Attribute("name"));
		weapon.spread = e->FirstChildElement("spread")->FloatText();
		weapon.rate = e->FirstChildElement("rate")->FloatText();
		weapon.damage = e->FirstChildElement("damage")->FloatText();
		weapon.range = e->FirstChildElement("range")->IntText();
		weapon.bullet_speed = e->FirstChildElement("bullet_speed")->IntText();
		weapon.bullet_count = e->FirstChildElement("bullet_count")->IntText();

		weapons.push_back(weapon);
	}
}

bool Weapons::mem(WEAPON_ID id)
{
	return id < weapons.size();
}

bool Weapons::get(WEAPON_ID id, Weapon *weapon)
{
	if (mem(id))
	{
		*weapon = weapons[id];
		return true;
	}

	return false;
}

int Weapons::write(NetworkFrame &frame)
{
	int size = frame.size();

	int n = (int)weapons.size();
	frame.append(&n, sizeof(n));

	for (Weapon weapon : weapons)
		weapon.write(frame);

	return frame.size() - size;
}

int Weapons::size()
{
	return sizeof(int) + (int)weapons.size() * Weapon::size();
}