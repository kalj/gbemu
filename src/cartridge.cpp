#include "cartridge.h"

#include "logging.h"

#include <fmt/core.h>

static uint32_t rom_size_from_code(uint8_t code) {
    switch (code) {
        case 0:
            return 32 * 1024; //          2 banks
        case 1:
            return 64 * 1024; //          4 banks
        case 2:
            return 128 * 1024; //         8 banks
        case 3:
            return 256 * 1024; //        16 banks
        case 4:
            return 512 * 1024; //        32 banks
        case 5:
            return 1024 * 1024; //       64 banks
        case 6:
            return 2 * 1024 * 1024; //  128 banks
        case 0x52:
            return 9 * 128 * 1024; //    72 banks
        case 0x53:
            return 10 * 128 * 1024; //   80 banks
        case 0x54:
            return 11 * 128 * 1024; //   96 banks
        default:
            return 0;
    }
}

static uint32_t ram_size_from_code(uint8_t code) {
    switch (code) {
        case 1:
            return 2 * 1024; //    1 bank
        case 2:
            return 8 * 1024; //    1 bank
        case 3:
            return 32 * 1024; //   4 banks
        case 4:
            return 128 * 1024; // 16 banks
        default:
            return 0;
    }
}

static std::string to_string(CartridgeType t) {
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

Cartridge::Cartridge(const std::vector<uint8_t> &rom_bytes)
    : rom(rom_bytes),
      type(static_cast<CartridgeType>(rom_bytes[0x147])),
      ram(ram_size_from_code(rom_bytes[0x149]), 0xff) {

    // for (int a = 0x104; a < 0x134; a++) {
    //     int i = a - 0x104;

    //     if (i % 16 == 0) {
    //         fmt::print("\n\t");
    //     }
    //     fmt::print("{:02X} ", this->rom[a]);
    //     // fmt::print("{:08b} ", rom[a]);
    // }
    // fmt::print("\n\n");

    const auto header_rom_size = rom_size_from_code(this->rom[0x148]);
    if (header_rom_size != this->rom.size()) {
        throw std::runtime_error(fmt::format("Mismatch in ROM size. Coded size = {} (code ${:02X}), actual size = {}",
                                             header_rom_size,
                                             this->rom[0x148],
                                             this->rom.size()));
    }

    uint8_t header_checksum_computed = 0;
    for (int i = 0x134; i < 0x14d; i++) {
        header_checksum_computed -= this->rom[i] + 1;
    }

    const uint8_t header_checksum_expected = this->rom[0x14d];
    if (header_checksum_computed != header_checksum_expected) {
        throw std::runtime_error(fmt::format("Invalid header checksum. Computed = {}, expected = {}",
                                             header_checksum_computed,
                                             header_checksum_expected));
    }

    uint16_t global_checksum_computed = 0;
    for (size_t i = 0; i < this->rom.size(); ++i) {
        if (i != 0x14e && i != 0x14f) {
            global_checksum_computed += this->rom[i];
        }
    }
    const uint16_t global_checksum_expected =
        static_cast<uint16_t>(this->rom[0x14e]) << 8 | static_cast<uint16_t>(this->rom[0x14f]);

    if (global_checksum_computed != global_checksum_expected) {
        // throw std::runtime_error(fmt::format("Invalid global checksum. Computed = {}, expected = {}", global_checksum_computed, global_checksum_expected));
        fmt::print("WARNING: Invalid global checksum. Computed = {}, expected = {}\n",
                   global_checksum_computed,
                   global_checksum_expected);
    }

    switch (this->type) {
        case CartridgeType::ROM_ONLY:
        case CartridgeType::ROM_MBC1:
        case CartridgeType::ROM_MBC2:
        case CartridgeType::ROM_MBC2_BATT:
        case CartridgeType::ROM_MMM01:
        case CartridgeType::ROM_MBC3_TIMER_BATT:
        case CartridgeType::ROM_MBC3:
        case CartridgeType::ROM_MBC5:
        case CartridgeType::ROM_MBC5_RUMBLE:
            if (this->ram.size() != 0) {
                throw std::runtime_error(fmt::format("Inconsistent header: Cartridge type is {} but RAM size is {}",
                                                     this->get_type_str(),
                                                     this->ram.size()));
            }
        default:
            break;
    }
}

int Cartridge::get_ram_size() const {
    return this->ram.size();
}

int Cartridge::get_rom_size() const {
    return this->rom.size();
}

int Cartridge::get_ram_banks() const {
    return (this->ram.size() + 8 * 1024 - 1) / (8 * 1024);
}

int Cartridge::get_rom_banks() const {
    return this->rom.size() / (16 * 1024);
}

std::string Cartridge::get_type_str() const {
    return to_string(this->type);
}

std::string Cartridge::get_title() const {
    return std::string((const char *)&this->rom[0x134], 0x143 - 0x134);
    // for (int i = 0x134; i < 0x143; i++) {
    //     fmt::format("{:c}", this->rom[i]);
    // }
}

uint8_t Cartridge::get_cgb_flag() const {
    return this->rom[0x143];
}

uint8_t Cartridge::get_sgb_flag() const {
    return this->rom[0x146];
}

uint8_t Cartridge::get_mask_rom_version() const {
    return this->rom[0x14c];
}

uint8_t Cartridge::get_destination_code() const {
    return this->rom[0x14a];
}

std::string Cartridge::get_licensee_code() const {
    return fmt::format("old=${:02X}, new=\"{:c}{:c}\"",
                       this->rom[0x14b],
                       rom[0x144] == 0 ? ' ' : rom[0x144],
                       rom[0x145] == 0 ? ' ' : rom[0x145]);
}

// bus operations

uint8_t Cartridge::read_rom(uint16_t addr) const {
    if (addr < 0x4000) {
        return this->rom[addr];
    } else if (addr < 0x8000) {
        const uint32_t actual_address = addr + uint32_t(0x4000) * (this->bank - 1);
        return this->rom[actual_address];
    } else {
        throw std::runtime_error(fmt::format("Invalid address passed to Cartridge::read_rom: ${:04X}", addr));
    }
}

void Cartridge::write_mbc(uint16_t addr, uint8_t data) {

    using enum CartridgeType;
    switch (this->type) {
        case ROM_ONLY:
            // throw std::runtime_error(fmt::format("Invalid write to MBC of ${:02X} at ${:04X} for cartridge type ${:02X} ({})", data, addr, this->type, to_string(this->type)));
            logging::warning(fmt::format("Invalid write to MBC of ${:02X} at ${:04X} for cartridge type ${:02X} ({})",
                                         data,
                                         addr,
                                         this->type,
                                         to_string(this->type)));
            break;
        case ROM_MBC1:
            if (addr >= 0x2000 || addr < 0x4000) {
                const auto actual_bank = data & 0x1f;
                this->bank             = actual_bank == 0 ? 1 : actual_bank;
                fmt::print("Switching to bank {}\n", this->bank);
            }
            break;
        default:
            throw std::runtime_error(
                fmt::format("write_mbc not implemented for cartridge type ${:02X} ({}), input: ${:04X}, ${:02X}",
                            this->type,
                            to_string(this->type),
                            addr,
                            data));
    }
}

uint8_t Cartridge::read_ram(uint16_t addr) const {
    throw std::runtime_error(fmt::format("read_ram not implemented (input: ${:04X})", addr));
}

void Cartridge::write_ram(uint16_t addr, uint8_t data) {
    throw std::runtime_error(fmt::format("write_ram not implemented (input: ${:04X}, ${:02X})", addr, data));
}

void Cartridge::dump_rom(std::ostream &os) {
    for (size_t i = 0; i < this->rom.size(); i++) {
        const uint16_t a = i + 0;
        if (a % 16 == 0) {
            os << fmt::format("\n${:04X}:", a);
        }

        os << fmt::format(" {:02X}", this->rom[i]);
    }
}
