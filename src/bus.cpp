#include "bus.h"

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


Bus::Bus(CartridgeType type, const std::vector<uint8_t> &cartridge_rom, uint32_t cram_size)
    : cartridge_type(type),
      rom(cartridge_rom),
      vram(8*1024, 0xff),
      cram(cram_size,0xff),
      wram(8*1024, 0xff),
      oam(160, 0),
      ioreg(128, 0),
      hram(127, 0xff)
    {}

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
        data = this->oam[addr-0xfe00];
    } else if(addr < 0xff00) {
        throw std::runtime_error(fmt::format("INVALID BUS READ AT ${:04X} (prohibited area)", addr));
    } else if(addr < 0xff80) {
        desc = "IO";
        data = this->ioreg[addr-0xff00];
        return data;
    } else if(addr == 0xffff) {
        desc = "IE reg";
        data = this->iereg;
    } else { // 0xff80 - 0xfffe
        desc = "HRAM";
        data = this->hram[addr-0xff80];
    }
    fmt::print("        BUS [${:04X}] -> ${:02X} ({})\n", addr, data, desc);
    return data;
}

void Bus::write(uint16_t addr, uint8_t data) {
    std::string desc = "";
    if(addr < 0x8000) {
        fmt::print("=========================================================\n");
        fmt::print("   WARNING: INVALID BUS WRITE AT ${:04X} (ROM), data=${:02X}\n", addr, data);
        fmt::print("=========================================================\n");
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
        this->oam[addr-0xfe00] = data;
    } else if(addr < 0xff00) {
        fmt::print("=====================================================================\n");
        fmt::print("   WARNING: INVALID BUS WRITE AT ${:04X} (prohibited area), data=${:02X}\n", addr, data);
        fmt::print("=====================================================================\n");
        return;
    } else if(addr < 0xff80) {
        desc = "IO";
        this->ioreg[addr-0xff00] = data;
        return;
    } else if(addr == 0xffff) {
        desc = "IE reg";
        this->iereg = data;
    } else { // 0xff80 - 0xfffe
        desc = "HRAM";
        this->hram[addr-0xff80] = data;
    }
    fmt::print("        BUS [${:04X}] <- ${:02X}  ({})\n", addr, data, desc);
}

void Bus::dump(std::ostream &os) const {

    os << "\nROM:";
    for(uint16_t i=0; i<this->rom.size(); i++) {
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
    for(uint16_t i=0; i<this->oam.size(); i++) {
        const uint16_t a = i+0xfe00;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->oam[i]);
    }

    os << "\nIO:";
    for(uint16_t i=0; i<this->ioreg.size(); i++) {
        const uint16_t a = i+0xff00;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->ioreg[i]);
    }

    os << "\nHRAM:";
    for(uint16_t i=0; i<this->hram.size(); i++) {
        const uint16_t a = i+0xff80;
        if(a%16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->hram[i]);
    }

    os << fmt::format("\nIE: {:02X}\n",this->iereg);
}
