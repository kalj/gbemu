#include "bus.h"

#include "ppu.h"
#include "sound.h"
#include "communication.h"
#include "controller.h"
#include "div_timer.h"
#include "interrupt_state.h"
#include "cartridge.h"

#include "logging.h"

#include <fmt/core.h>

Bus::Bus(Cartridge &cart,
         Controller &cntl,
         Communication &comm,
         DivTimer &dt,
         Sound &snd,
         Ppu &ppu,
         InterruptState &is)
    : vram(8 * 1024, 0xff),
      wram(8 * 1024, 0xff),
      hram(127, 0xff),
      cartridge(cart),
      controller(cntl),
      communication(comm),
      div_timer(dt),
      sound(snd),
      ppu(ppu),
      int_state(is) {}

uint8_t Bus::read(uint16_t addr) const {
    uint8_t data = 0xff;
    std::string desc = "";
    if(addr < 0x8000) {
        desc = "ROM";
        data = this->cartridge.read_rom(addr);
    } else if(addr < 0xa000) {
        desc = "VRAM";
        data = this->vram[addr-0x8000];
    } else if(addr < 0xc000) {
        desc = "Cartridge RAM";
        data = this->cartridge.read_ram(addr-0xa000);
    } else if(addr < 0xe000) {
        desc = "WRAM";
        data = this->wram[addr-0xc000];
    } else if(addr < 0xfe00) {
        desc = "ECHO RAM";
        data = this->wram[addr-0xe000];
    } else if(addr < 0xfea0) {
        desc = "OAM";
        data = this->ppu.read_oam(addr-0xfe00);
    } else if(addr < 0xff00) {
        throw std::runtime_error(fmt::format("INVALID BUS READ AT ${:04X} (prohibited area)", addr));
    } else if(addr == 0xff0f || addr == 0xffff) {
        desc = "InterruptState";
        data = this->int_state.read_reg(addr-0xff00);
    } else if(addr < 0xff80) {
        desc = "IO";
        const uint8_t regid = addr-0xff00;
        if(regid == 0x00) {
            data = this->controller.read_reg();
        } else if(regid >= 0x01 && regid <= 0x02) {
            data = this->communication.read_reg(regid);
        } else if(regid >= 0x04 && regid <= 0x07) {
            data = this->div_timer.read_reg(regid);
        } else if(regid >= 0x10 && regid <= 0x3f) {
            data = this->sound.read_reg(regid);
        } else if(regid >= 0x40 && regid <= 0x4b) {
            data = this->ppu.read_reg(regid);
        } else {
            logging::warning(fmt::format("=====================================================================\n"));
            logging::warning(fmt::format("   WARNING: INVALID IO REGISTER READ AT ${:04X}\n", addr));
            logging::warning(fmt::format("=====================================================================\n"));
        }
    } else { // 0xff80 - 0xfffe
        desc = "HRAM";
        data = this->hram[addr-0xff80];
    }
    logging::debug(fmt::format("        BUS [${:04X}] -> ${:02X} ({})\n", addr, data, desc));
    return data;
}

void Bus::write(uint16_t addr, uint8_t data) {
    std::string desc = "";
    if(addr < 0x8000) {
        desc = "MBC";
        this->cartridge.write_mbc(addr, data);
    } else if(addr < 0xa000) {
        desc = "VRAM";
        this->vram[addr-0x8000] = data;
    } else if(addr < 0xc000) {
        this->cartridge.write_ram(addr-0xa000, data);
    } else if(addr < 0xe000) {
        desc = "WRAM";
        this->wram[addr-0xc000] = data;
    } else if(addr < 0xfe00) {
        desc = "ECHO RAM";
        this->wram[addr-0xe000] = data;
    } else if(addr < 0xfea0) {
        desc = "OAM";
        this->ppu.write_oam(addr-0xfe00,data);
    } else if(addr < 0xff00) {
        logging::warning(fmt::format("=====================================================================\n"));
        logging::warning(fmt::format("   WARNING: INVALID BUS WRITE AT ${:04X} (prohibited area), data=${:02X}\n", addr, data));
        logging::warning(fmt::format("=====================================================================\n"));
        return;
    } else if(addr == 0xff0f || addr == 0xffff) {
        desc = "InterruptState";
        this->int_state.write_reg(addr-0xff00, data);
    } else if(addr < 0xff80) {
        desc = "IO";
        const uint8_t regid = addr-0xff00;
        if(regid == 0x00) {
            this->controller.write_reg(data);
        } else if(regid >= 0x01 && regid <= 0x02) {
            this->communication.write_reg(regid, data);
        } else if(regid >= 0x04 && regid <= 0x07) {
            this->div_timer.write_reg(regid, data);
        } else if(regid >= 0x10 && regid <= 0x3f) {
            this->sound.write_reg(regid, data);
        } else if(regid >= 0x40 && regid <= 0x4b) {
            this->ppu.write_reg(regid, data);
        } else {
            logging::warning(fmt::format("=====================================================================\n"));
            logging::warning(fmt::format("   WARNING: INVALID IO REGISTER WRITE AT ${:04X} data=${:02X}\n", addr, data));
            logging::warning(fmt::format("=====================================================================\n"));
            return;
        }
    } else { // 0xff80 - 0xfffe
        desc = "HRAM";
        this->hram[addr-0xff80] = data;
    }
    logging::debug(fmt::format("        BUS [${:04X}] <- ${:02X}  ({})\n", addr, data, desc));
}

void Bus::dump(std::ostream &os) const {

    os << "\nCartridge RAM:\n";
    this->cartridge.dump_ram(os);

    os << "\nVRAM:";
    for(uint16_t i=0; i<this->vram.size(); i++) {
        const uint16_t a = i+0x8000;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->vram[i]);
    }

    os << "\nWRAM:";
    for(uint16_t i=0; i<this->wram.size(); i++) {
        const uint16_t a = i+0xc000;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->wram[i]);
    }

    os << "\nOAM:";
    this->ppu.dump_oam(os);

    os << "\nIO:\n";
    this->controller.dump(os);
    this->communication.dump(os);
    this->div_timer.dump(os);
    this->sound.dump(os);
    this->ppu.dump_regs(os);

    os << "\nHRAM:";
    for(uint16_t i=0; i<this->hram.size(); i++) {
        const uint16_t a = i+0xff80;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->hram[i]);
    }

    os << "\nInterruptState:\n";
    this->int_state.dump(os);
}
