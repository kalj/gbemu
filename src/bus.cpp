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
    if(addr < 0x8000) {
        fmt::print("        BUS read @ ${:04X} (ROM): ${:02X}\n", addr, this->rom[addr]);
        return this->rom[addr];
    } else if(addr < 0xa000) {
        const auto a = addr-0x8000;
        fmt::print("        BUS read @ ${:04X} (VRAM): ${:02X}\n", addr, this->vram[a]);
        return this->vram[a];
    } else if(addr < 0xc000) {
        fmt::print("INVALID BUS READ AT ${:04X} (external RAM)\n", addr);
        throw std::runtime_error("XRAM READ");
    } else if(addr < 0xe000) {
        const auto a = addr-0xc000;
        fmt::print("        BUS read @ ${:04X} (WRAM): ${:02X}\n", addr, this->wram[a]);
        return this->wram[a];
    } else if(addr < 0xfe00) {
        const auto a = addr-0xe000;
        fmt::print("        BUS read @ ${:04X} (ECHO RAM): ${:02X}\n", addr, this->wram[a]);
        return this->wram[a];
    } else if(addr < 0xfea0) {
        const auto a = addr-0xfe00;
        fmt::print("        BUS read @ ${:04X} (OAM): ${:02X}\n", addr, this->oam[a]);
        return this->oam[a];
    } else if(addr < 0xff00) {
        fmt::print("INVALID BUS READ AT ${:04X} (prohibited area)\n", addr);
        throw std::runtime_error("BUS PROHIB");
    } else if(addr < 0xff80) {
        const auto a = addr-0xff00;
        fmt::print("        BUS read @ ${:04X} (IO): ${:02X}\n", addr, this->ioreg[a]);
        return this->ioreg[a];
    } else if(addr == 0xffff) {
        fmt::print("        BUS read @ ${:04X} (IE reg): ${:02X}\n", addr, this->iereg);
        return this->iereg;
    } else { // 0xff80 - 0xfffe
        const auto a = addr-0xff80;
        fmt::print("        BUS read @ ${:04X} (HRAM): ${:02X}\n", addr, this->hram[a]);
        return this->hram[a];
    }
}

void Bus::write(uint16_t addr, uint8_t data) {
    fmt::print("        BUS write of ${:02X} @ ${:04X} ", data, addr);
    if(addr < 0x8000) {
        fmt::print("\n");
        throw std::runtime_error(fmt::format("INVALID BUS WRITE AT ${:04X} (ROM)", addr));
    } else if(addr < 0xa000) {
        const auto a = addr-0x8000;
        fmt::print("(VRAM)\n");
        this->vram[a] = data;
    } else if(addr < 0xc000) {
        fmt::print("\n");
        throw std::runtime_error(fmt::format("INVALID BUS WRITE AT ${:04X} (external RAM)", addr));
    } else if(addr < 0xe000) {
        const auto a = addr-0xc000;
        fmt::print("(WRAM)\n");
        this->wram[a] = data;
    } else if(addr < 0xfe00) {
        const auto a = addr-0xe000;
        fmt::print("(ECHO RAM)\n");
        this->wram[a] = data;
    } else if(addr < 0xfea0) {
        const auto a = addr-0xfe00;
        fmt::print("(OAM)\n");
        this->oam[a] = data;
    } else if(addr < 0xff00) {
        fmt::print("\n");
        throw std::runtime_error(fmt::format("INVALID BUS WRITE AT ${:04X} (prohibited area)", addr));
    } else if(addr < 0xff80) {
        const auto a = addr-0xff00;
        fmt::print("(IO)\n");
        this->ioreg[a] = data;
    } else if(addr == 0xffff) {
        fmt::print("(IE reg)\n");
        this->iereg = data;
    } else { // 0xff80 - 0xfffe
        const auto a = addr-0xff80;
        fmt::print("(HRAM)\n");
        this->hram[a] = data;
    }
}
