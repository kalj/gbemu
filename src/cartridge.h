#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

enum class CartridgeType : uint8_t {
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

class Mbc;

class Cartridge {
public:
    Cartridge(const std::vector<uint8_t> &cartridge_rom);
    ~Cartridge();

    // getters
    int get_ram_size() const;
    int get_rom_size() const;
    int get_ram_banks() const;
    int get_rom_banks() const;
    std::string get_type_str() const;
    std::string get_title() const;
    uint8_t get_cgb_flag() const;
    uint8_t get_sgb_flag() const;
    uint8_t get_mask_rom_version() const;
    uint8_t get_destination_code() const;
    std::string get_licensee_code() const;

    // bus operations
    uint8_t read_rom(uint16_t addr) const;
    void write_mbc(uint16_t addr, uint8_t data);
    uint8_t read_ram(uint16_t addr) const;
    void write_ram(uint16_t addr, uint8_t data);

    void dump_ram(std::ostream &os);

private:
    std::unique_ptr<Mbc> mbc;
    const std::vector<uint8_t> &rom;
    CartridgeType type{CartridgeType::ROM_ONLY};
    std::vector<uint8_t> ram;
};

#endif /* CARTRIDGE_H */
