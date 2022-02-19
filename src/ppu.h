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
    Ppu() : oam(160, 0) {}

    void do_tick(std::vector<uint32_t> &buf, const Bus &bus, InterruptState &int_state);

    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);

    uint8_t read_oam(uint8_t regid) const {
        return this->oam[regid];
    }
    void write_oam(uint8_t regid, uint8_t data) {
        this->oam[regid] = data;
    }

    void dump_regs(std::ostream &os) const;
    void dump_oam(std::ostream &os) const;
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

    // oam
    std::vector<uint8_t> oam;

    // other state
    bool prev_stat_interrupt_line{false};
    unsigned int lx{~0u};
    uint8_t      mode{2};
};


#endif /* PPU_H */
