#ifndef SOUND_H
#define SOUND_H

#include <cstdint>
#include <iosfwd>

class Sound {
public:

    void reset();

    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;
};

#endif /* SOUND_H */
