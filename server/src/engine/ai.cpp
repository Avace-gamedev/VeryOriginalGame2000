#include "engine/ai.h"

AI::AI(World *world) : world(world){};

int AI::getPlayerIndexById(ID id)
{
    for (unsigned int i = 0; i < entities.size(); i++)
        if (entities[i]->id == id)
            return i;
    return -1;
}

LinePath::LinePath(World *world, double size) : AI(world), size(size)
{
    ctrl_left.movement = MOVE_LEFT;
    ctrl_right.movement = MOVE_RIGHT;
}

Control LinePath::makeNextControl(Entity *entity)
{
    int i = getPlayerIndexById(entity->id);
    if (i < 0)
    {
        entities.push_back(entity);

        Vec2f pos = entity->topleft();
        initial_pos.push_back(pos);
        last_ctrl.push_back(nullptr);

        i = (int)entities.size() - 1;
    }

    Vec2f pos = entities[i]->topleft();

    Control *res;

    if (pos.x >= initial_pos[i].x + size)
        res = &ctrl_left;
    else if (pos.x <= initial_pos[i].x)
        res = &ctrl_right;
    else if (last_ctrl[i])
        res = last_ctrl[i];
    else
        res = &ctrl_right;

    last_ctrl[i] = res;

    return *res;
}