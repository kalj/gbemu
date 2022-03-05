#ifndef DIV_TIMER_H
#define DIV_TIMER_H

#include <cstdint>
#include <iosfwd>

class InterruptState;

class DivTimer {
public:
    void reset();

    void do_tick(uint64_t clock, InterruptState &int_state);
    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;

private:
    uint8_t div{0};
    uint8_t timer{0};
    uint8_t timer_modulo{0};
    bool timer_enable{false};
    uint8_t clock_select{0};
};

#endif /* DIV_TIMER_H */
