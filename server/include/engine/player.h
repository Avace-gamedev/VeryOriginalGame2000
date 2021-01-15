#pragma once

#include <windows.h>

#include "engine/entity.h"
#include "engine/controller.h"

class Player : public Entity
{
protected:
    void applyControl(const Control *control) override;

public:
    sockaddr_in addr;
    tick_t client_tick = 0;
    bool ready = false;

    Player(ID id, sockaddr_in addr, std::string name = "__player__", float max_health = 100);

    const std::vector<Control *> update(const tick_t current_tick, const TilemapDesc *map) override;
    void rememberControl(Control &control);
};