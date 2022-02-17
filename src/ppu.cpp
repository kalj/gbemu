#include "ppu.h"

#include "log.h"
#include "bus.h"

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


#define LCD_WIDTH           160
#define N_DOTS_PER_SCANLINE 456

#define LCD_HEIGHT        144
#define N_YBLANK          10
#define N_SCANLINES_TOTAL (LCD_HEIGHT + N_YBLANK)

// H: 144: 154 scanlines  (coord : index LY ( FF44))
// W: 160: 456 dots per scanline -> 70224 dots -> 4 MiHz / 70224 = 59.727 5Hz

//  Mode cycle: 22 333 00  22 333 00 .. 111111

void Ppu::do_tick(std::vector<uint32_t> &buf, Bus &bus) {
    const auto lcdc = bus.read(REG_LCDC);
    const auto stat = bus.read(REG_STAT);
    const auto scx  = bus.read(REG_SCX);
    const auto scy  = bus.read(REG_SCY);
    const auto lyc  = bus.read(REG_LYC);

    if((lcdc & LCDC_LCD_ENABLE) == 0) {
        // lcd / ppu disabled
        return;
    }

    uint8_t ly  = bus.read(REG_LY);

    // do pixel stuff

    if(this->mode == 2) {
        // do OAM scan

        // gather 10 sprites from OAM with Y such that it is visible considering the value of ly
        // selected_sprites
    } else if(this->mode == 3) {
        // do nothing ?
    } else if(this->mode == 0) {
        // do nothing ?
    }

    // at some point, fill in row of buf

    this->lx = (this->lx + 1) % N_DOTS_PER_SCANLINE;
    if (this->lx == 0) {
        ly = (ly + 1) % N_SCANLINES_TOTAL;
    }

    if (this->lx == 0) { // -> mode 2 if just entered a new row
        this->mode = 2;
        // TODO: Set interrupt?
    } else if(this->lx == 80) { // -> mode 3 after 80 dots of mode 2
        this->mode = 3;
        // TODO: Set interrupt?
    }

    // switch to mode 1 if just entered Y blank region
    if(ly == LCD_HEIGHT && this->lx==0) {
        this->mode = 1;
        // TODO: Set interrupt?
    }

    // check if ly == lyc, set in stat
    // TODO: set some kind of interrupt?
    const bool lyc_ly = ly == lyc;

    // update registers
    bus.write(REG_STAT, (stat&0xf8) | (lyc_ly?0x4:0) | this->mode);
    bus.write(REG_LY, ly);

}
