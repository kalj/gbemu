#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cstdint>
#include <iosfwd>

class Controller {
public:
    // void set_states(uint8_t d_u_l_r_st_sel_b_a);
    uint8_t read_reg() const;
    void write_reg(uint8_t data);
    void dump(std::ostream &os) const;
private:
    bool directions_selected{false};
    bool actions_selected{false};
    uint8_t direction_buttons_state{0xff};
    uint8_t action_buttons_state{0xff};
};

#endif /* CONTROLLER_H */
