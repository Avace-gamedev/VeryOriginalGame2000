#pragma once

#include "common/deftypes.h"
#include "engine/entity.h"
#include "engine/controller.h"
#include "engine/world.h"

#include <vector>

class AI
{
protected:
    World *world;
    std::vector<Entity *> entities;

    int getPlayerIndexById(ID id);

public:
    AI(World *world);
    virtual Control makeNextControl(Entity *entity) { return {0}; };
};

class LinePath : public AI
{
    std::vector<Vec2f> initial_pos;
    std::vector<Control *> last_ctrl;

    Control ctrl_left;
    Control ctrl_right;

public:
    double size;

    LinePath(World *world, double size);

    Control makeNextControl(Entity *entity) override;
};