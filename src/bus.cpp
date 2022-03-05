#include "bus.h"

#include "ppu.h"
#include "sound.h"
#include "communication.h"
#include "controller.h"
#include "div_timer.h"
#include "interrupt_state.h"

#include "logging.h"

#include <fmt/core.h>

std::string to_string(CartridgeType t) {
    using enum CartridgeType;
    switch (t) {
        case ROM_ONLY:
            return "ROM_ONLY";
        case ROM_MBC1:
            return "ROM_MBC1";
        case ROM_MBC1_RAM:
            return "ROM_MBC1_RAM";
        case ROM_MBC1_RAM_BATT:
            return "ROM_MBC1_RAM_BATT";
        case ROM_MBC2:
            return "ROM_MBC2";
        case ROM_MBC2_BATT:
            return "ROM_MBC2_BATT";
        case ROM_RAM:
            return "ROM_RAM";
        case ROM_RAM_BATT:
            return "ROM_RAM_BATT";
        case ROM_MMM01:
            return "ROM_MMM01";
        case ROM_MMM01_SRAM:
            return "ROM_MMM01_SRAM";
        case ROM_MMM01_SRAM_BATT:
            return "ROM_MMM01_SRAM_BATT";
        case ROM_MBC3_TIMER_BATT:
            return "ROM_MBC3_TIMER_BATT";
        case ROM_MBC3_TIMER_RAM_BATT:
            return "ROM_MBC3_TIMER_RAM_BATT";
        case ROM_MBC3:
            return "ROM_MBC3";
        case ROM_MBC3_RAM_:
            return "ROM_MBC3_RAM_";
        case ROM_MBC3_RAM_BATT:
            return "ROM_MBC3_RAM_BATT";
        case ROM_MBC5:
            return "ROM_MBC5";
        case ROM_MBC5_RAM_:
            return "ROM_MBC5_RAM_";
        case ROM_MBC5_RAM_BATT:
            return "ROM_MBC5_RAM_BATT";
        case ROM_MBC5_RUMBLE:
            return "ROM_MBC5_RUMBLE";
        case ROM_MBC5_RUMBLE_RAM:
            return "ROM_MBC5_RUMBLE_RAM";
        case ROM_MBC5_RUMBLE_RAM_BATT:
            return "ROM_MBC5_RUMBLE_RAM_BATT";
        case POCKET_CAMERA:
            return "POCKET_CAMERA";
        case BANDAI_TAMA5:
            return "BANDAI_TAMA5";
        case HUDSON_HUC3:
            return "HUDSON_HUC3";
        case HUDSON_HUC1:
            return "HUDSON_HUC1";
        default:
            return "INVALID";
    }
}

Bus::Bus(CartridgeType type,
         const std::vector<uint8_t> &cartridge_rom,
         uint32_t ram_size,
         Controller &cntl,
         Communication &comm,
         DivTimer &dt,
         Sound &snd,
         Ppu &ppu,
         InterruptState &is)
    : cartridge_type(type),
      rom(cartridge_rom),
      vram(8 * 1024, 0xff),
      cram(ram_size, 0xff),
      wram(8 * 1024, 0xff),
      hram(127, 0xff),
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
        data = this->rom[addr];
    } else if(addr < 0xa000) {
        desc = "VRAM";
        data = this->vram[addr-0x8000];
    } else if(addr < 0xc000) {
        throw std::runtime_error(fmt::format("INVALID BUS READ AT ${:04X} (external RAM)", addr));
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
        logging::warning(fmt::format("=========================================================\n"));
        logging::warning(fmt::format("   WARNING: INVALID BUS WRITE AT ${:04X} (ROM), data=${:02X}\n", addr, data));
        logging::warning(fmt::format("=========================================================\n"));
        return;
    } else if(addr < 0xa000) {
        desc = "VRAM";
        this->vram[addr-0x8000] = data;
    } else if(addr < 0xc000) {
        throw std::runtime_error(fmt::format("INVALID BUS WRITE AT ${:04X} (external RAM), data=${:02X}", addr, data));
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
    } else if(addr == 0xffff) {
    } else { // 0xff80 - 0xfffe
        desc = "HRAM";
        this->hram[addr-0xff80] = data;
    }
    logging::debug(fmt::format("        BUS [${:04X}] <- ${:02X}  ({})\n", addr, data, desc));
}

void Bus::dump(std::ostream &os) const {

    os << "\nROM:";
    for(size_t i=0; i<this->rom.size(); i++) {
        const uint16_t a = i+0;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->rom[i]);
    }

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
