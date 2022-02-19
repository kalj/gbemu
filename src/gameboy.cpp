#include "gameboy.h"

#include <fmt/core.h>

uint32_t rom_size_from_code(uint8_t code) {
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

uint32_t ram_size_from_code(uint8_t code) {
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

Gameboy::Gameboy(std::vector<uint8_t> &rom_contents)
    : cartridge_type(static_cast<CartridgeType>(rom_contents[0x147])),
      cartridge_ram_size(ram_size_from_code(rom_contents[0x149])),
      rom(rom_contents),
      bus(cartridge_type, rom, cartridge_ram_size,
          controller, communication, div_timer, sound, ppu, interrupt_state),
      pixel_buffer(LCD_WIDTH*LCD_HEIGHT)
{}

void Gameboy::print_header() const {

    fmt::print("Read Cartridge ROM of size {} Bytes\n", this->rom.size());

    fmt::print("Nintendo graphic:");
    for (int a = 0x104; a < 0x134; a++) {
        int i = a - 0x104;

        if (i % 16 == 0) {
            fmt::print("\n\t");
        }
        fmt::print("{:02X} ", this->rom[a]);
        // fmt::print("{:08b} ", rom[a]);
    }
    fmt::print("\n\n");

    fmt::print("Title: ");
    for (int i = 0x134; i < 0x143; i++) {
        fmt::print("{:c}", this->rom[i]);
    }
    fmt::print("\n\n");

    fmt::print("Color byte (${:04X}): ${:02X}\n\n", 0x143, this->rom[0x143]);

    if (this->rom[0x144] == 0 && this->rom[0x145] == 0) {
        fmt::print("No new-style license\n\n");
    } else {
        fmt::print("License (new):  {:c}{:c}\n\n", rom[0x144], this->rom[0x145]);
    }

    fmt::print("SGB indicator (${:04X}): ${:02X}\n\n", 0x146, this->rom[0x146]);

    fmt::print("Cartridge type (${:04X}): ${:02X} ({})\n\n",
               0x147,
               this->cartridge_type,
               to_string(this->cartridge_type));

    auto rom_size = rom_size_from_code(this->rom[0x148]);
    if(rom_size != this->rom.size()) {
        throw std::runtime_error(fmt::format("Mismatch in cartridge rom size. Coded size = {}, actual size = {}\n",
                                             rom_size, this->rom.size()));
    }
    fmt::print("ROM size code (${:04X}): ${:02X} ({} KiB, {} banks)\n\n",
               0x148,
               this->rom[0x148],
               rom_size / 1024,
               rom_size / (16 * 1024));

    fmt::print("RAM size code (${:04X}): ${:02X} ({} KiB, {} banks)\n\n",
               0x149,
               this->rom[0x149],
               this->cartridge_ram_size / 1024,
               (this->cartridge_ram_size + 8 * 1024 - 1) / (8 * 1024));

    fmt::print("Destination code (${:04X}): ${:02X} ({})\n\n",
               0x14a,
               this->rom[0x14a],
               this->rom[0x14a] == 0 ? "Japanese" : "Non-Japanese");

    fmt::print("License code (old) (${:04X}): ${:02X}\n\n", 0x14b, this->rom[0x14b]);

    fmt::print("Mask ROM Version (${:04X}): ${:02X}\n\n", 0x14c, this->rom[0x14c]);

    uint8_t header_checksum_computed = 0;
    for (int i = 0x134; i < 0x14d; i++) {
        header_checksum_computed -= this->rom[i] + 1;
    }

    uint8_t header_checksum_expected = this->rom[0x14d];
    fmt::print("Header Checksum (${:04X}): ${:02X} ({})\n\n",
               0x14d,
               header_checksum_expected,
               header_checksum_computed == header_checksum_expected ? "Valid" : "Invalid");

    uint16_t global_checksum_computed = 0;
    for (size_t i = 0; i < this->rom.size(); ++i) {
        if (i != 0x14e && i != 0x14f) {
            global_checksum_computed += this->rom[i];
        }
    }
    uint16_t global_checksum_expected = static_cast<uint16_t>(this->rom[0x14e]) << 8 | static_cast<uint16_t>(this->rom[0x14f]);
    fmt::print("Global Checksum (${:04X}): ${:04X} ({})\n",
               0x14e,
               global_checksum_expected,
               global_checksum_computed == global_checksum_expected ? "Valid" : "Invalid");
}

void Gameboy::reset() {
    this->cpu.reset();
}

void Gameboy::do_tick() {
    this->cpu.do_tick(this->bus, this->interrupt_state);

    for(int j=0; j<4; j++) {
        // fmt::print(" === [PPU cycle = {}]\n", 4*i+j);
        this->ppu.do_tick(this->pixel_buffer, this->bus, this->interrupt_state);
    }
}

void Gameboy::dump(std::ostream &os) const {
    os << "Registers:\n";
    cpu.dump(os);
    os << "\nMemory dump:\n";
    bus.dump(os);
}