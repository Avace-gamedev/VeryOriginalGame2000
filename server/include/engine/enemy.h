#pragma once

#include <string>

#include "common/vector.hpp"

struct Enemy
{
    std::string name;
    Vec2f pos;
    int difficulty;
};