#include "controller.h"

#include <fmt/core.h>

// TODO: set interrupt (bit 4 in IF) at high-to-low transitions (of stuff that is active!)

uint8_t Controller::read_reg() const {
    uint8_t res = 0xff;
    if(this->actions_selected) {
        res &= (0xf0|this->action_buttons_state);
    }
    if(this->directions_selected) {
        res &= (0xf0|this->direction_buttons_state);
    }
    return res;
}

void Controller::write_reg(uint8_t data) {
    this->directions_selected = (data&0x10) == 0; // bit 4 is 0
    this->actions_selected    = (data&0x20) == 0; // bit 5 is 0
}

void Controller::dump(std::ostream &os) const {
    os << fmt::format("Controller state:\n");
    os << fmt::format("  Directions selected:     {}\n", this->directions_selected);
    os << fmt::format("  Actions selected:        {}\n", this->actions_selected);
    os << fmt::format("  Direction buttons state: {:04b}\n", this->direction_buttons_state);
    os << fmt::format("  Action buttons state:    {:04b}\n", this->action_buttons_state);
    os << fmt::format("  P1/JOYP [0xff00]:        {:02X}\n",  this->read_reg());
}
