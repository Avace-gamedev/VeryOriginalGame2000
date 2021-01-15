#pragma once

#include <memory>
#include <vector>

#include "common/time.h"
#include "common/deftypes.h"
#include "common/ring.hpp"

class Entity;
class AI;

#define CTRL_MSG_SIZE 5 // in bytes

struct Control
{
    tick_t tick;
    uint8_t movement;
    bool change_weapon;
    int new_weapon_i;
    bool run;
    bool shoot;
    float facing_angle;
};

#define MOVE_UP 0b1000
#define MOVE_DOWN 0b0100
#define MOVE_LEFT 0b0010
#define MOVE_RIGHT 0b0001

#define UP(movement) ((movement & MOVE_UP) == MOVE_UP)
#define DOWN(movement) ((movement & MOVE_DOWN) == MOVE_DOWN)
#define LEFT(movement) ((movement & MOVE_LEFT) == MOVE_LEFT)
#define RIGHT(movement) ((movement & MOVE_RIGHT) == MOVE_RIGHT)

class Controller
{
protected:
    Ring<Control> control_history;
    int last_ctrl_tick = -1;

    // server-side client tick at which getControls has returned controls
    // used to limit the rate at which control frames are processed
    tick_t last_call_tick = 0;

    Control *getNextControl(tick_t from);

public:
    Controller();

    virtual void update(Entity *entity, tick_t tick){};

    // clients updates faster than server,
    // server needs to read several controls per step
    const std::vector<Control *> getControls(tick_t tick);

    void registerControl(Control &ctrl);
};

class PlayerController : public Controller
{
public:
    PlayerController();
};

class AIController : public Controller
{
    std::shared_ptr<AI> ai;

public:
    AIController(std::shared_ptr<AI> ai);

    void update(Entity *entity, tick_t current_tick) override;
};