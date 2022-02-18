#include "ppu.h"

#include "log.h"
#include "bus.h"
#include "interrupt_state.h"

#include <fmt/core.h>
#include <stdexcept>

#define REG_DMA 0xFF46

#define REG_LCDC 0xFF40 // LCDC: LCD Control                     (PPU RO)
#define REG_STAT 0xFF41 // STAT: LCD Status                      (Bits 0-2 RW, 3-6 RO)
#define REG_SCY  0xFF42 // SCY: Scroll Y                         (PPU RO)
#define REG_SCX  0xFF43 // SCX: Scroll X                         (PPU RO)
#define REG_LY   0xFF44 // LY: LCD Y Coordinate                  (PPU RW)
#define REG_LYC  0xFF45 // LYC: LY Compare                       (PPU RO)
#define REG_BGP  0xFF47 // BGP:  BG Palette Data    (Non CGB)    (PPU RO)
#define REG_OBP0 0xFF48 // OBP0: Obj Palette 0 Data (Non CGB)    (PPU RO)
#define REG_OBP1 0xFF49 // OBP1: Obj Palette 1 Data (Non CGB)    (PPU RO)
#define REG_WY   0xFF4A // WY: Window Y Position                 (PPU RO)
#define REG_WX   0xFF4B // WX: Window X Position + 7             (PPU RO)


#define LCDC_BG_WIN_ENABLE     0b00000001
#define LCDC_OBJ_ENABLE        0b00000010
#define LCDC_OBJ_SIZE          0b00000100
#define LCDC_BG_TMAP_AREA      0b00001000
#define LCDC_BG_WIN_TDATA_AREA 0b00010000
#define LCDC_WIN_ENABLE        0b00100000
#define LCDC_WIN_TMAP_AREA     0b01000000
#define LCDC_LCD_ENABLE        0b10000000

uint8_t Ppu::read_reg(uint8_t regid) const {
    switch (regid) {
        case 0x40:
            return this->lcdc;
        case 0x41:
            return this->stat;
        case 0x42:
            return this->scy;
        case 0x43:
            return this->scx;
        case 0x44:
            return this->ly;
        case 0x45:
            return this->lyc;

        case 0x47:
            return this->bgp;
        case 0x48:
            return this->obp0;
        case 0x49:
            return this->obp1;
        case 0x4A:
            return this->wy;
        case 0x4B:
            return this->wx;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to Ppu: {}", regid));
    }

}
void Ppu::write_reg(uint8_t regid, uint8_t data) {
    switch (regid) {
        case 0x40:
            this->lcdc = data;
            break;
        case 0x41:
            this->stat = data;
            break;
        case 0x42:
            this->scy = data;
            break;
        case 0x43:
            this->scx = data;
            break;
        // case 0x44: prohibited to write to LY!
        case 0x45:
            this->lyc = data;
            break;

        case 0x47:
            this->bgp = data;
            break;
        case 0x48:
            this->obp0 = data;
            break;
        case 0x49:
            this->obp1 = data;
            break;
        case 0x4A:
            this->wy = data;
            break;
        case 0x4B:
            this->wx = data;
            break;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to Ppu: {}", regid));
    }
}
void Ppu::dump(std::ostream &os) const {
    os << fmt::format("LCDC [0xFF40] {:02X}\n", this->lcdc);
    os << fmt::format("STAT [0xFF41] {:02X}\n", this->stat);
    os << fmt::format("SCY  [0xFF42] {:02X}\n", this->scy);
    os << fmt::format("SCX  [0xFF43] {:02X}\n", this->scx);
    os << fmt::format("LY   [0xFF44] {:02X}\n", this->ly);
    os << fmt::format("LYC  [0xFF45] {:02X}\n", this->lyc);

    os << fmt::format("OBP  [0xFF47] {:02X}\n", this->bgp);
    os << fmt::format("OBP0 [0xFF48] {:02X}\n", this->obp0);
    os << fmt::format("OBP1 [0xFF49] {:02X}\n", this->obp1);
    os << fmt::format("WY   [0xFF4A] {:02X}\n", this->wy);
    os << fmt::format("WX   [0xFF4B] {:02X}\n", this->wx);
}


#define N_DOTS_PER_SCANLINE 456

#define N_YBLANK          10
#define N_SCANLINES_TOTAL (LCD_HEIGHT + N_YBLANK)

// H: 144: 154 scanlines  (coord : index LY ( FF44))
// W: 160: 456 dots per scanline -> 70224 dots -> 4 MiHz / 70224 = 59.727 5Hz

//  Mode cycle: 22 333 00  22 333 00 .. 111111

void Ppu::do_tick(std::vector<uint32_t> &buf, Bus &bus, InterruptState &int_state) {

    if((this->lcdc & LCDC_LCD_ENABLE) == 0) {
        // lcd / ppu disabled
        return;
    }

    //---------------------------------------------------------------
    // update to go to next dot
    //---------------------------------------------------------------
    this->lx = (this->lx + 1) % N_DOTS_PER_SCANLINE;
    if (this->lx == 0) {
        this->ly = (this->ly + 1) % N_SCANLINES_TOTAL;
    }

    // handle region/mode transitions and interrupts
    if (ly < LCD_HEIGHT && this->lx == 0) { // -> mode 2 if just entered a new row
        this->mode = 2;
    }

    if(ly < LCD_HEIGHT && this->lx == 80) { // -> mode 3 after 80 dots of mode 2
        this->mode = 3;
    }

    if(ly == LCD_HEIGHT && this->lx==0) { // mode 1 if just entered Y blank region
        this->mode = 1;
        // set VBLANK interrupt
        int_state.set_if_vblank();
    }

    // check if ly == lyc, set in stat
    const bool lyc_ly = this->ly == this->lyc;

    const bool stat_interrupt_line = ((this->mode == 0) && (this->stat & 0x8)) ||
                                     ((this->mode == 1) && (this->stat & 0x10)) ||
                                     ((this->mode == 2) && (this->stat & 0x20)) || (lyc_ly && (this->stat & 0x40));

    if(stat_interrupt_line && !this->prev_stat_interrupt_line) {
        // set the STAT interrupt
        int_state.set_if_stat();
    }
    this->prev_stat_interrupt_line = stat_interrupt_line;

    // update registers
    this->stat = (this->stat&0xf8) | (lyc_ly?0x4:0) | this->mode;

    // do pixel stuff
    if(this->mode == 2) {
        // do OAM scan

        // gather 10 sprites from OAM with Y such that it is visible considering the value of ly
        // selected_sprites
    } else if(this->mode == 3) {

        // do the drawing

        // at some point switch to mode 0
    } else if(this->mode == 0) {
        // do nothing ?
    }
}
