#include "engine/controller.h"

#include "loguru/loguru.hpp"

#include "engine/game_config.h"
#include "engine/ai.h"

Controller::Controller() : control_history(ACK_SIZE * 8){};

void Controller::registerControl(Control &ctrl)
{
    if (!control_history.mem(ctrl.tick))
        control_history.write(ctrl.tick, ctrl);
    else
        LOG_F(WARNING, "control %d already exists (discarded)", ctrl.tick);
}

Control *Controller::getNextControl(tick_t from)
{
    if (control_history.size() <= 0)
        return nullptr;

    tick_t last_tick_received = control_history.get(0)->tick;

    if (from > last_tick_received)
        return nullptr;

    while (!control_history.mem(from) && from < last_tick_received)
        from++;

    return control_history.getById(from);
}

const std::vector<Control *> Controller::getControls(tick_t current_tick)
{
    std::vector<Control *> res;

    if (last_ctrl_tick == 0)
        // no control yet
        if (control_history.size() == 0)
            return res;
        else
        {
            Control *ctrl = control_history.get(0);
            last_ctrl_tick = ctrl->tick;
            last_call_tick = current_tick;
            res.push_back(ctrl);
            return res;
        }

    // time advanced since last call ?
    // if not, no new control available
    if (current_tick > last_call_tick)
    {
        // number of ticks elapsed since last call, this is the max number of ticks to process
        tick_t n_ticks = current_tick - last_call_tick;

        // read at most n_ticks ticks and store them in res
        tick_t aux = last_ctrl_tick;
        for (tick_t i = 0; i < n_ticks; i++)
        {
            // returns nullptr if no control available
            Control *next_ctrl = getNextControl(aux + 1);

            if (!next_ctrl)
                break;

            res.push_back(next_ctrl);
            aux = next_ctrl->tick;
        }

        last_ctrl_tick = aux;
    }

    if (res.size() > 0)
        last_call_tick = current_tick;

    return res;
}

PlayerController::PlayerController() : Controller(){};

AIController::AIController(std::shared_ptr<AI> ai) : ai(ai) {}

void AIController::update(Entity *entity, tick_t current_tick)
{
    Control ctrl = ai->makeNextControl(entity);
    ctrl.tick = current_tick;

    // LOG_F(INFO, "tick %d, other move:%s%s%s%s", current_tick,
    //       (UP(ctrl.movement) ? " UP" : ""),
    //       (DOWN(ctrl.movement) ? " DOWN" : ""),
    //       (LEFT(ctrl.movement) ? " LEFT" : ""),
    //       (RIGHT(ctrl.movement) ? " RIGHT" : ""));

    registerControl(ctrl);
}