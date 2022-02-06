#include "cpu.h"
#include "bus.h"

#include <fmt/core.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>


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

int main(int argc, char **argv) {

    if (argc < 2) {
        fmt::print("Cartridge ROM path expected\n");
        return 1;
    }

    auto cartridge_rom_file = std::filesystem::path(argv[1]);

    if (!std::filesystem::exists(cartridge_rom_file)) {
        fmt::print("No Cartridge ROM found at \"{}\"\n", cartridge_rom_file.string());
        return 1;
    }

    std::ifstream infile(cartridge_rom_file, std::ios_base::binary);

    std::vector<uint8_t> cartridge_rom_contents(std::istreambuf_iterator<char>(infile), {});
    const auto &rom = cartridge_rom_contents;

    fmt::print("Read Cartridge ROM of size {} Bytes\n", cartridge_rom_contents.size());

    fmt::print("Nintendo graphic:");
    for (int a = 0x104; a < 0x134; a++) {
        int i = a - 0x104;

        if (i % 16 == 0) {
            fmt::print("\n\t");
        }
        fmt::print("{:02X} ", rom[a]);
        // fmt::print("{:08b} ", rom[a]);
    }
    fmt::print("\n\n");

    fmt::print("Title: ");
    for (int i = 0x134; i < 0x143; i++) {
        fmt::print("{:c}", rom[i]);
    }
    fmt::print("\n\n");

    fmt::print("Color byte (${:04X}): ${:02X}\n\n", 0x143, rom[0x143]);

    if (rom[0x144] == 0 && rom[0x145] == 0) {
        fmt::print("No new-style license\n\n");
    } else {
        fmt::print("License (new):  {:c}{:c}\n\n", rom[0x144], rom[0x145]);
    }

    fmt::print("SGB indicator (${:04X}): ${:02X}\n\n", 0x146, rom[0x146]);

    auto cartridge_type = static_cast<CartridgeType>(rom[0x147]);
    fmt::print("Cartridge type (${:04X}): ${:02X} ({})\n\n",
               0x147,
               cartridge_type,
               to_string(cartridge_type));

    auto rom_size = rom_size_from_code(rom[0x148]);
    if(rom_size != cartridge_rom_contents.size()) {
        fmt::print("Mismatch in cartridge rom size. Coded size = {}, actual size = {}\n",
                   rom_size, cartridge_rom_contents.size());
        return 1;
    }
    fmt::print("ROM size code (${:04X}): ${:02X} ({} KiB, {} banks)\n\n",
               0x148,
               rom[0x148],
               rom_size / 1024,
               rom_size / (16 * 1024));

    auto ram_size = ram_size_from_code(rom[0x149]);
    fmt::print("RAM size code (${:04X}): ${:02X} ({} KiB, {} banks)\n\n",
               0x149,
               rom[0x149],
               ram_size / 1024,
               (ram_size + 8 * 1024 - 1) / (8 * 1024));

    fmt::print("Destination code (${:04X}): ${:02X} ({})\n\n",
               0x14a,
               rom[0x14a],
               rom[0x14a] == 0 ? "Japanese" : "Non-Japanese");

    fmt::print("License code (old) (${:04X}): ${:02X}\n\n", 0x14b, rom[0x14b]);

    fmt::print("Mask ROM Version (${:04X}): ${:02X}\n\n", 0x14c, rom[0x14c]);

    uint8_t header_checksum_computed = 0;
    for (int i = 0x134; i < 0x14d; i++) {
        header_checksum_computed -= rom[i] + 1;
    }

    uint8_t header_checksum_expected = rom[0x14d];
    fmt::print("Header Checksum (${:04X}): ${:02X} ({})\n\n",
               0x14d,
               header_checksum_expected,
               header_checksum_computed == header_checksum_expected ? "Valid" : "Invalid");

    uint16_t global_checksum_computed = 0;
    for (int i = 0; i < rom.size(); ++i) {
        if (i != 0x14e && i != 0x14f) {
            global_checksum_computed += rom[i];
        }
    }
    uint16_t global_checksum_expected = static_cast<uint16_t>(rom[0x14e]) << 8 | static_cast<uint16_t>(rom[0x14f]);
    fmt::print("Global Checksum (${:04X}): ${:04X} ({})\n",
               0x14e,
               global_checksum_expected,
               global_checksum_computed == global_checksum_expected ? "Valid" : "Invalid");

    Bus bus{cartridge_type, cartridge_rom_contents, ram_size};

    Cpu cpu;
    cpu.reset();

    // let rom_dev = bus_devices::Rom::new(0, &rom);
    // let mut bus = bus_devices::Bus::new(rom_dev);

    fmt::print("------------------------------------------------------\n");
    fmt::print("Starting execution\n\n");

    for(int i=0;;i++) {
        cpu.do_instruction(bus);
    }

    return 0;
}
