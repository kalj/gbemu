#ifndef PPU_H
#define PPU_H

#include "ibus.h"

#include <cstdint>
#include <iosfwd>
#include <vector>

#define LCD_WIDTH  160
#define LCD_HEIGHT 144

#define OAM_SIZE 160

class InterruptState;

class Ppu {
public:
    Ppu() : oam(OAM_SIZE, 0) {
    }

    void reset();

    void do_tick(std::vector<uint32_t> &buf, const IBus &bus, InterruptState &int_state);

    bool dma_is_active() const;
    void tick_dma(uint64_t clock, IBus &bus);

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

    // dma state
    uint16_t dma_src_base;
    uint8_t dma_n_bytes_left{0};

    // other rendering state
    bool prev_stat_interrupt_line{false};
    unsigned int lx{~0u};
    uint8_t mode{2};
};

#endif /* PPU_H */
