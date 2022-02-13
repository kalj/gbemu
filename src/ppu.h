#ifndef PPU_H
#define PPU_H

#include <vector>
#include <cstdint>

class Bus;

class Ppu {
public:
    void do_tick(std::vector<uint32_t> &buf, Bus &bus);
private:
    unsigned int lx;
    uint8_t      mode;

};


#endif /* PPU_H */
