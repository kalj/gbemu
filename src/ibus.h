#ifndef IBUS_H
#define IBUS_H

#include <cstdint>

class IBus {
public:
    virtual uint8_t read(uint16_t addr) const       = 0;
    virtual void write(uint16_t addr, uint8_t data) = 0;
};

#endif /* IBUS_H */
