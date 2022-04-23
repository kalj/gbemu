#include "controller.h"

#include <fmt/core.h>

// TODO: set interrupt (bit 4 in IF) at high-to-low transitions (of stuff that is active!)

namespace gb_controller {
    void Controller::reset() {
        this->directions_selected     = false;
        this->actions_selected        = false;
        this->direction_buttons_state = 0xff;
        this->action_buttons_state    = 0xff;
    }

    void Controller::set_button_state(Button button, State state) {
        switch (button) {
            case Button::RIGHT:
            case Button::LEFT:
            case Button::UP:
            case Button::DOWN:
                if (state == State::DOWN) {
                    this->direction_buttons_state &= ~(1 << static_cast<int>(button));
                } else {
                    this->direction_buttons_state |= (1 << static_cast<int>(button));
                }
                break;
            case Button::A:
            case Button::B:
            case Button::SELECT:
            case Button::START:
                if (state == State::DOWN) {
                    this->action_buttons_state &= ~(1 << (static_cast<int>(button) - static_cast<int>(Button::A)));
                } else {
                    this->action_buttons_state |= (1 << (static_cast<int>(button) - static_cast<int>(Button::A)));
                }
                break;
            default:
                break;
        }
    }

    uint8_t Controller::read_reg() const {
        uint8_t res = 0xff;
        if (this->actions_selected) {
            res &= (0xf0 | this->action_buttons_state);
        }
        if (this->directions_selected) {
            res &= (0xf0 | this->direction_buttons_state);
        }
        return res;
    }

    void Controller::write_reg(uint8_t data) {
        this->directions_selected = (data & 0x10) == 0; // bit 4 is 0
        this->actions_selected    = (data & 0x20) == 0; // bit 5 is 0
    }

    void Controller::dump(std::ostream &os) const {
        os << fmt::format("Controller state:\n");
        os << fmt::format("  Directions selected:     {}\n", this->directions_selected);
        os << fmt::format("  Actions selected:        {}\n", this->actions_selected);
        os << fmt::format("  Direction buttons state: {:04b}\n", this->direction_buttons_state);
        os << fmt::format("  Action buttons state:    {:04b}\n", this->action_buttons_state);
        os << fmt::format("  P1/JOYP [0xff00]:        {:02X}\n", this->read_reg());
    }
} // namespace gb_controller
