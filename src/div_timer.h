#ifndef DIV_TIMER_H
#define DIV_TIMER_H

#include <cstdint>
#include <iosfwd>

class DivTimer {
public:
    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;

private:
    uint8_t div{0};
    uint8_t tima{0};
    uint8_t tma{0};
    uint8_t tac{0};
};

#endif /* DIV_TIMER_H */
