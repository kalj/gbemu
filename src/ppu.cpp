#include "ppu.h"

#include "interrupt_state.h"
#include "logging.h"

#include <fmt/core.h>
#include <stdexcept>

#define LCDC_BG_WIN_ENABLE     0b00000001
#define LCDC_OBJ_ENABLE        0b00000010
#define LCDC_OBJ_SIZE          0b00000100
#define LCDC_BG_TMAP_AREA      0b00001000
#define LCDC_BG_WIN_TDATA_AREA 0b00010000
#define LCDC_WIN_ENABLE        0b00100000
#define LCDC_WIN_TMAP_AREA     0b01000000
#define LCDC_LCD_ENABLE        0b10000000

#define N_DOTS_PER_SCANLINE 456

#define N_YBLANK          10
#define N_SCANLINES_TOTAL (LCD_HEIGHT + N_YBLANK)

void Ppu::reset() {
    this->write_reg(0x40, 0x91); //   ; LCDC
    this->write_reg(0x42, 0x00); //   ; SCY
    this->write_reg(0x43, 0x00); //   ; SCX
    this->write_reg(0x45, 0x00); //   ; LYC
    this->write_reg(0x47, 0xFC); //   ; BGP
    this->write_reg(0x48, 0xFF); //   ; OBP0
    this->write_reg(0x49, 0xFF); //   ; OBP1
    this->write_reg(0x4A, 0x00); //   ; WY
    this->write_reg(0x4B, 0x00); //   ; WX
}

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

        case 0x46:
            this->dma_src_base = static_cast<uint16_t>(data) << 8;
            logging::debug("Starting DMA from ${:04X}\n", this->dma_src_base);
            this->dma_n_bytes_left = OAM_SIZE;
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
    os << fmt::format("PPU state:\n");
    os << fmt::format("  LCDC [0xFF40] {:02X}\n", this->lcdc);
    os << fmt::format("  STAT [0xFF41] {:02X}\n", this->stat);
    os << fmt::format("  SCY  [0xFF42] {:02X}\n", this->scy);
    os << fmt::format("  SCX  [0xFF43] {:02X}\n", this->scx);
    os << fmt::format("  LY   [0xFF44] {:02X}\n", this->ly);
    os << fmt::format("  LYC  [0xFF45] {:02X}\n", this->lyc);

    os << fmt::format("  BGP  [0xFF47] {:02X}\n", this->bgp);
    os << fmt::format("  OBP0 [0xFF48] {:02X}\n", this->obp0);
    os << fmt::format("  OBP1 [0xFF49] {:02X}\n", this->obp1);
    os << fmt::format("  WY   [0xFF4A] {:02X}\n", this->wy);
    os << fmt::format("  WX   [0xFF4B] {:02X}\n", this->wx);
}

void Ppu::dump_oam(std::ostream &os) const {
    for (uint16_t i = 0; i < this->oam.size(); i++) {
        const uint16_t a = i + 0xfe00;
        if (a % 16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->oam[i]);
    }
}

bool Ppu::dma_is_active() const {
    return this->dma_n_bytes_left != 0;
}

void Ppu::tick_dma(uint64_t clock, IBus &bus) {

    if(clock%4 != 0) {
        // divide the clock by 4 to get 1MiHz (2^20 Hz)
        return;
    }

    if (this->dma_n_bytes_left == 0) {
        return;
    }
    // copy from high to low address...
    this->dma_n_bytes_left--;
    logging::debug("\t\t\t\t\t\t\t\t Performing DMA transfer from ${:04X} to OAM at ${:04X}\n",
                               this->dma_src_base + this->dma_n_bytes_left,
                               this->dma_n_bytes_left + 0xfe00);
    this->oam[this->dma_n_bytes_left] = bus.read(this->dma_src_base + this->dma_n_bytes_left);
}

// H: 144: 154 scanlines  (coord : index LY ( FF44))
// W: 160: 456 dots per scanline -> 70224 dots -> 4 MiHz / 70224 = 59.727 5Hz

//  Mode cycle: 22 333 00  22 333 00 .. 111111

void Ppu::do_tick(std::vector<uint32_t> &buf, const IBus &bus, InterruptState &int_state) {

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
            int8_t object_indices[10];
            int8_t object_row[10];
            int n_object_indices=0;
            // foreach object in oam, gather (up to) 10 relevant ones
            const auto tile_height = this->lcdc&LCDC_OBJ_SIZE ? 16 : 8;
            for(int i=0; i<40; i++) {
                const auto ypos = this->oam[4*i];

                if(this->ly+16 >= ypos && this->ly+16 < ypos+tile_height) {
                    object_row[n_object_indices] = this->ly+16-ypos;
                    object_indices[n_object_indices++] = i;
                    // save y coord in tile
                }
            }

            // background tilemap coordinate
            // background tile index
            const uint8_t bgy = this->scy + this->ly; // wrapping ok
            const uint8_t bg_tm_iy = bgy / 8; // 0-31
            const uint8_t bg_td_iy = bgy % 8; // 0-7
            const uint16_t tile_map_area_base_idx = (this->lcdc&LCDC_BG_TMAP_AREA)?0x9c00 : 0x9800;

            // do the drawing
            for(int i=0; i<LCD_WIDTH; i++) {
                // for this pixel, check background tiles, window tiles, and each of the potential objects/sprites.
                // set this pixel to the value based on the above

                uint8_t value = 0;

                if(this->lcdc&LCDC_BG_WIN_ENABLE) { // background / window enabled
                    // coordinate within the background tile map:
                    uint8_t bgx = this->scx + i; // wrapping ok
                    const uint8_t bg_tm_ix = bgx / 8; // 0-31
                    const uint8_t bg_td_ix = bgx % 8; // 0-7

                    uint8_t tile_idx = bus.read(tile_map_area_base_idx + 32 * bg_tm_iy + bg_tm_ix);
                    uint16_t tile_data_area_base_idx = 0x8000;
                    if((this->lcdc & LCDC_BG_WIN_TDATA_AREA) == 0) {
                        tile_idx += 128;
                        tile_data_area_base_idx = 0x8800;
                    }

                    const uint16_t tile_address = tile_data_area_base_idx + tile_idx*16;

                    const auto tile_row_lsb = bus.read(tile_address + 2*bg_td_iy);
                    const auto tile_row_msb = bus.read(tile_address + 2*bg_td_iy+1);
                    const auto color_id_lsb = (tile_row_lsb >>(7-bg_td_ix))&0x1;
                    const auto color_id_msb = (tile_row_msb >>(7-bg_td_ix))&0x1;
                    const uint8_t color_id = (color_id_msb<<1)|color_id_lsb;

                    value = (this->bgp>>(color_id<<1))&0x3;

                    // if(this->lcdc&LCDC_WIN_ENABLE) { // window enabled
                    // }
                }

                if(this->lcdc&LCDC_OBJ_ENABLE) { // object/sprite enabled
                    // loop over objects until a non-transparent pixel value has been found,
                    // and save the object index along the way

                    // int8_t object_index = -1;
                    for(int i=0; i<n_object_indices; i++) {
                        const auto oi = object_indices[i];
                        const auto xpos = this->oam[4*oi + 1];
                        if(this->lx+8 >= xpos && this->lx < xpos) {
                            // the sprite covers this pixel
                            const auto tile_index = this->oam[4*oi + 2];

                            // get coord in tile:
                            const auto sprite_iy = object_row[i];
                            const auto sprite_ix = this->lx+8-xpos;

                            // TODO: adjust for flip etc

                            const auto sprite_row_lsb = bus.read(0x8000+tile_index*16 + 2*sprite_iy);
                            const auto sprite_row_msb = bus.read(0x8000+tile_index*16 + 2*sprite_iy+1);
                            const auto color_id_lsb = (sprite_row_lsb>>(7-sprite_ix))&0x1;
                            const auto color_id_msb = (sprite_row_msb>>(7-sprite_ix))&0x1;
                            const uint8_t color_id = (color_id_msb<<1)|color_id_lsb;

                            if(color_id == 0) { // transparent, so skip this sprite and continue
                                continue;
                            }

                            const auto attributes = this->oam[4*oi + 3];

                            // get actual color using palette
                            // if not transparent, assign value to `value` and break the loop
                            // TODO: check for background on top

                            if((attributes & (1<<7)) != (1<<7)) {
                                value = ((attributes&(1<<4) ? this->obp1 : this->obp0)>>color_id)&0x3;
                            }

                            break;
                        }
                    }
                }

                value = 255-(value<<6); // 0-3 -> 0-192 -> 192-0
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
