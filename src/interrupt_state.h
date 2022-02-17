#ifndef INTERRUPT_STATE_H
#define INTERRUPT_STATE_H

#include <cstdint>
#include <iosfwd>

class InterruptState {
public:
    void set_if_vblank();
    void set_if_stat();

    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;
private:
    uint8_t if_reg{0};
    uint8_t ie_reg{0};
};


#endif /* INTERRUPT_STATE_H */
