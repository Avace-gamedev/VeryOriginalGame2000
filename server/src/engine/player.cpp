#include "engine/player.h"

Player::Player(ID id, sockaddr_in addr, std::string name, float max_health) : Entity(id, PLAYER, name, max_health, new PlayerController()), addr(addr) {}

const std::vector<Control *> Player::update(const tick_t current_tick, const TilemapDesc *map)
{
    std::vector<Control *> ctrls = Entity::update(current_tick, map);

    tick_t max_tick = client_tick;
    for (Control *ctrl : ctrls)
        max_tick = max(max_tick, ctrl->tick);
    client_tick = max_tick;

    return ctrls;
}

void Player::applyControl(const Control* control)
{
    Entity::applyControl(control);
}

void Player::rememberControl(Control &ctrl)
{
    controller->registerControl(ctrl);
}