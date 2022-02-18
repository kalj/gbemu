#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <vector>
#include <iosfwd>

#define LCD_WIDTH         160
#define LCD_HEIGHT        144

class Bus;
class InterruptState;

class Ppu {
public:
    void do_tick(std::vector<uint32_t> &buf, Bus &bus, InterruptState &int_state);

    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;
private:

    // register
    uint8_t lcdc{0};
    uint8_t stat{0};
    uint8_t scy{0};
    uint8_t scx{0};
    uint8_t ly{0};
    uint8_t lyc{0};

    uint8_t bgp{0};
    uint8_t obp0{0};
    uint8_t obp1{0};
    uint8_t wy{0};
    uint8_t wx{0};

    // other state
    bool prev_stat_interrupt_line{false};
    unsigned int lx{~0u};
    uint8_t      mode{2};
};


#endif /* PPU_H */
