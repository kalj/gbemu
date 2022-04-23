#include "bus.h"

#include "cartridge.h"
#include "communication.h"
#include "controller.h"
#include "div_timer.h"
#include "interrupt_state.h"
#include "ppu.h"
#include "sound.h"

#include "logging.h"

#include <fmt/core.h>

Bus::Bus(Cartridge &cart, gb_controller::Controller &cntl, Communication &comm, DivTimer &dt, Sound &snd, Ppu &ppu, InterruptState &is)
    : vram(8 * 1024, 0xff),
      wram(8 * 1024, 0xff),
      hram(127, 0xff),
      cartridge(cart),
      controller(cntl),
      communication(comm),
      div_timer(dt),
      sound(snd),
      ppu(ppu),
      int_state(is) {
}

uint8_t Bus::read(uint16_t addr) const {
    uint8_t data = 0xff;
    if (addr < 0x8000) {
        data = this->cartridge.read_rom(addr);
        logging::debug("        BUS [${:04X}] -> ${:02X} (ROM)\n", addr, data);
    } else if (addr < 0xa000) {
        data = this->vram[addr - 0x8000];
        logging::debug("        BUS [${:04X}] -> ${:02X} (VRAM)\n", addr, data);
    } else if (addr < 0xc000) {
        data = this->cartridge.read_ram(addr - 0xa000);
        logging::debug("        BUS [${:04X}] -> ${:02X} (Cartridge RAM)\n", addr, data);
    } else if (addr < 0xe000) {
        data = this->wram[addr - 0xc000];
        logging::debug("        BUS [${:04X}] -> ${:02X} (WRAM)\n", addr, data);
    } else if (addr < 0xfe00) {
        data = this->wram[addr - 0xe000];
        logging::debug("        BUS [${:04X}] -> ${:02X} (ECHO RAM)\n", addr, data);
    } else if (addr < 0xfea0) {
        data = this->ppu.read_oam(addr - 0xfe00);
        logging::debug("        BUS [${:04X}] -> ${:02X} (OAM)\n", addr, data);
    } else if (addr < 0xff00) {
        throw std::runtime_error(fmt::format("INVALID BUS READ AT ${:04X} (prohibited area)", addr));
    } else if (addr == 0xff0f || addr == 0xffff) {
        data = this->int_state.read_reg(addr - 0xff00);
        logging::debug("        BUS [${:04X}] -> ${:02X} (Interrupt State)\n", addr, data);
    } else if (addr < 0xff80) {
        const uint8_t regid = addr - 0xff00;
        if (regid == 0x00) {
            data = this->controller.read_reg();
        } else if (regid >= 0x01 && regid <= 0x02) {
            data = this->communication.read_reg(regid);
        } else if (regid >= 0x04 && regid <= 0x07) {
            data = this->div_timer.read_reg(regid);
        } else if (regid >= 0x10 && regid <= 0x3f) {
            data = this->sound.read_reg(regid);
        } else if (regid >= 0x40 && regid <= 0x4b) {
            data = this->ppu.read_reg(regid);
        } else {
            logging::warning("=====================================================================\n");
            logging::warning("   WARNING: INVALID IO REGISTER READ AT ${:04X}\n", addr);
            logging::warning("=====================================================================\n");
        }
        logging::debug("        BUS [${:04X}] -> ${:02X} (IO)\n", addr, data);
    } else { // 0xff80 - 0xfffe
        data = this->hram[addr - 0xff80];
        logging::debug("        BUS [${:04X}] -> ${:02X} (HRAM)\n", addr, data);
    }
    return data;
}

void Bus::write(uint16_t addr, uint8_t data) {
    if (addr < 0x8000) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (MBC)", addr, data);
        this->cartridge.write_mbc(addr, data);
    } else if (addr < 0xa000) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (VRAM)", addr, data);
        this->vram[addr - 0x8000] = data;
    } else if (addr < 0xc000) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (Cartridge RAM)", addr, data);
        this->cartridge.write_ram(addr - 0xa000, data);
    } else if (addr < 0xe000) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (WRAM)", addr, data);
        this->wram[addr - 0xc000] = data;
    } else if (addr < 0xfe00) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (ECHO RAM)", addr, data);
        this->wram[addr - 0xe000] = data;
    } else if (addr < 0xfea0) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (OAM)", addr, data);
        this->ppu.write_oam(addr - 0xfe00, data);
    } else if (addr < 0xff00) {
        logging::warning("=====================================================================\n");
        logging::warning("   WARNING: INVALID BUS WRITE AT ${:04X} (prohibited area), data=${:02X}\n", addr, data);
        logging::warning("=====================================================================\n");
        return;
    } else if (addr == 0xff0f || addr == 0xffff) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (InterruptState)", addr, data);
        this->int_state.write_reg(addr - 0xff00, data);
    } else if (addr < 0xff80) {
        logging::debug("        BUS [${:04X}] <- ${:02X}  (IO)", addr, data);
        const uint8_t regid = addr - 0xff00;
        if (regid == 0x00) {
            this->controller.write_reg(data);
        } else if (regid >= 0x01 && regid <= 0x02) {
            this->communication.write_reg(regid, data);
        } else if (regid >= 0x04 && regid <= 0x07) {
            this->div_timer.write_reg(regid, data);
        } else if (regid >= 0x10 && regid <= 0x3f) {
            this->sound.write_reg(regid, data);
        } else if (regid >= 0x40 && regid <= 0x4b) {
            this->ppu.write_reg(regid, data);
        } else {
            logging::warning("=====================================================================\n");
            logging::warning("   WARNING: INVALID IO REGISTER WRITE AT ${:04X} data=${:02X}\n", addr, data);
            logging::warning("=====================================================================\n");
            return;
        }
    } else { // 0xff80 - 0xfffe
        logging::debug("        BUS [${:04X}] <- ${:02X}  (HRAM)", addr, data);
        this->hram[addr - 0xff80] = data;
    }
}

void Bus::dump(std::ostream &os) const {

    os << "\nCartridge RAM:\n";
    this->cartridge.dump_ram(os);

    os << "\nVRAM:";
    for (uint16_t i = 0; i < this->vram.size(); i++) {
        const uint16_t a = i + 0x8000;
        if (a % 16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->vram[i]);
    }

    os << "\nWRAM:";
    for (uint16_t i = 0; i < this->wram.size(); i++) {
        const uint16_t a = i + 0xc000;
        if (a % 16 == 0) {
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
    for (uint16_t i = 0; i < this->hram.size(); i++) {
        const uint16_t a = i + 0xff80;
        if (a % 16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->hram[i]);
    }

    os << "\nInterruptState:\n";
    this->int_state.dump(os);
}
