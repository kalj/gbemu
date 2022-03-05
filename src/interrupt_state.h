#ifndef INTERRUPT_STATE_H
#define INTERRUPT_STATE_H

#include <cstdint>
#include <iosfwd>

enum class InterruptCause {
    VBLANK   = 0,
    LCD_STAT = 1,
    TIMER    = 2,
    SERIAL   = 3,
    JOYPAD   = 4,
};

std::string interrupt_cause_to_string(InterruptCause ic);

class InterruptState {
public:
    void reset();

    void set_if_bit(InterruptCause ic);
    void clear_if_bit(InterruptCause ic);

    uint8_t get_interrupts() const {
        return ie_reg & if_reg;
    }

    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;
private:
    uint8_t if_reg{0};
    uint8_t ie_reg{0};
};


#endif /* INTERRUPT_STATE_H */
