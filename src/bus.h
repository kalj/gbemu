#ifndef BUS_H
#define BUS_H

#include <string>
#include <vector>

enum class CartridgeType: uint8_t {
    ROM_ONLY                 = 0x0,
    ROM_MBC1                 = 0x1,
    ROM_MBC1_RAM             = 0x2,
    ROM_MBC1_RAM_BATT        = 0x3,
    ROM_MBC2                 = 0x5,
    ROM_MBC2_BATT            = 0x6,
    ROM_RAM                  = 0x8,
    ROM_RAM_BATT             = 0x9,
    ROM_MMM01                = 0xb,
    ROM_MMM01_SRAM           = 0xc,
    ROM_MMM01_SRAM_BATT      = 0xd,
    ROM_MBC3_TIMER_BATT      = 0xf,
    ROM_MBC3_TIMER_RAM_BATT  = 0x10,
    ROM_MBC3                 = 0x11,
    ROM_MBC3_RAM_            = 0x12,
    ROM_MBC3_RAM_BATT        = 0x13,
    ROM_MBC5                 = 0x19,
    ROM_MBC5_RAM_            = 0x1a,
    ROM_MBC5_RAM_BATT        = 0x1b,
    ROM_MBC5_RUMBLE          = 0x1c,
    ROM_MBC5_RUMBLE_RAM      = 0x1d,
    ROM_MBC5_RUMBLE_RAM_BATT = 0x1e,
    POCKET_CAMERA            = 0x1f,
    BANDAI_TAMA5             = 0xfd,
    HUDSON_HUC3              = 0xfe,
    HUDSON_HUC1              = 0xff,
};

std::string to_string(CartridgeType t);

class Bus {
public:
    Bus(CartridgeType type, const std::vector<uint8_t> &cartridge_rom, uint32_t ram_size);
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t data);
    void dump(std::ostream &os) const;

private:
    CartridgeType cartridge_type;
    const std::vector<uint8_t> &rom;
    std::vector<uint8_t> vram;
    std::vector<uint8_t> cram;
    std::vector<uint8_t> wram;
    std::vector<uint8_t> oam;
    std::vector<uint8_t> ioreg;
    std::vector<uint8_t> hram;
    uint8_t iereg{0};
    // std::vector<std::unique_ptr<BusDevice>> bus_devices;
};


#endif /* BUS_H */
