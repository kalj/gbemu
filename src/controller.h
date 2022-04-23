#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cstdint>
#include <iosfwd>

namespace gb_controller {
    enum class Button {
        RIGHT  = 0,
        LEFT   = 1,
        UP     = 2,
        DOWN   = 3,
        A      = 4,
        B      = 5,
        SELECT = 6,
        START  = 7,
    };

    enum class State {
        DOWN = 0,
        UP = 1,
    };

    class Controller {
    public:
        void reset();
        uint8_t read_reg() const;
        void write_reg(uint8_t data);
        void dump(std::ostream &os) const;
        void set_button_state(Button button, State state);

    private:
        bool directions_selected{false};
        bool actions_selected{false};
        uint8_t direction_buttons_state{0xff};
        uint8_t action_buttons_state{0xff};
    };
} // namespace gb_controller

#endif /* CONTROLLER_H */
