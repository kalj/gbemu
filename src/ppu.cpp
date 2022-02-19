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
void Ppu::dump_regs(std::ostream &os) const {
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

void Ppu::dump_oam(std::ostream &os) const {
    for(uint16_t i=0; i<this->oam.size(); i++) {
        const uint16_t a = i+0xfe00;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->oam[i]);
    }
}

#define N_DOTS_PER_SCANLINE 456

#define N_YBLANK          10
#define N_SCANLINES_TOTAL (LCD_HEIGHT + N_YBLANK)

// H: 144: 154 scanlines  (coord : index LY ( FF44))
// W: 160: 456 dots per scanline -> 70224 dots -> 4 MiHz / 70224 = 59.727 5Hz

//  Mode cycle: 22 333 00  22 333 00 .. 111111

void Ppu::do_tick(std::vector<uint32_t> &buf, const Bus &bus, InterruptState &int_state) {

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
        int_state.set_if_bit(InterruptCause::VBLANK);
    }

    // check if ly == lyc, set in stat
    const bool lyc_ly = this->ly == this->lyc;

    const bool stat_interrupt_line = ((this->mode == 0) && (this->stat & 0x8)) ||
                                     ((this->mode == 1) && (this->stat & 0x10)) ||
                                     ((this->mode == 2) && (this->stat & 0x20)) || (lyc_ly && (this->stat & 0x40));

    if(stat_interrupt_line && !this->prev_stat_interrupt_line) {
        // set the STAT interrupt
        int_state.set_if_bit(InterruptCause::LCD_STAT);
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

        if(this->lx == 90) {
            // int8_t object_indices[10];
            // int n_object_indices=0;
            // // foreach object in oam, gather (up to) 10 relevant ones
            // const auto tile_height = this->lcdc&(1<<2) ? 16 : 8;
            // for(int i=0; i<40; i++) {
            //     const auto ypos = this->oam[4*i];

            //     if(this->ly+16 >= ypos && this->ly+16 < ypos+tile_height) {
            //         object_indices[n_object_indices++] = i;
            //     }
            // }

            // background tilemap coordinate
            // background tile index
            const uint8_t bgy = this->scx + this->ly; // wrapping ok
            const uint8_t bg_tm_iy = bgy / 8; // 0-31
            const uint8_t bg_td_iy = bgy % 8; // 0-7

            // do the drawing
            for(int i=0; i<LCD_WIDTH; i++) {
                // for this pixel, check background tiles, window tiles, and each of the potential objects/sprites.
                // set this pixel to the value based on the above

                uint8_t value = 0xff;

                if(this->lcdc&0x01) { // background / window enabled
                    // coordinate within the background tile map:
                    uint8_t bgx = this->scx + i; // wrapping ok
                    const uint8_t bg_tm_ix = bgx / 8; // 0-31
                    const uint8_t bg_td_ix = bgx % 8; // 0-7

                    const uint16_t tile_map_area_base_idx = (this->lcdc&(1<<3))?0x9c00 : 0x9800;
                    uint8_t tile_idx = bus.read(tile_map_area_base_idx + 32 * bg_tm_iy + bg_tm_ix);
                    uint16_t tile_data_area_base_idx = 0x8000;
                    if((this->lcdc & (1 << 4)) == 0) {
                        tile_idx -= 128;
                        tile_data_area_base_idx = 0x8800;
                    }

                    const uint8_t tile_row_lsb = (bus.read(tile_data_area_base_idx*16 + 2*bg_td_iy)>>(7-bg_td_ix))&0x1;
                    const uint8_t tile_row_msb = (bus.read(tile_data_area_base_idx*16 + 2*bg_td_iy+1)>>(7-bg_td_ix))&0x1;
                    const uint8_t color_id = (tile_row_msb<<1)|tile_row_lsb;

                    value = 255-(((this->bgp>>color_id)&0x3) << 6); // 0-3 -> 0-192 -> 192-0

                    // if(this->lcdc&(1<<5)) { // window enabled
                    // }
                }

                // if(this->lcdc&(1<<1)) { // object/sprite enabled
                //     int8_t object_index = -1;
                //     for(int i=0; i<n_object_indices; i++) {
                //         const auto xpos = this->oam[4*object_indices[i] + 1];
                //         if(this->lx+8 >= xpos && this->lx < xpos) {

                //             object_index = 0;
                //             break;
                //         }



                // }

                buf[LCD_WIDTH*this->ly + i] = value<<24 | value<<16 | value<<8 | 0xff;
            }
        }

        if(this->lx == 250) {
            // at some point switch to mode 0
            this->mode = 0;
        }

    } else if(this->mode == 0) {
        // do nothing ?
    }
}
