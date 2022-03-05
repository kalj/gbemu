#include "gameboy.h"

#include <fmt/core.h>

Gameboy::Gameboy(const std::vector<uint8_t> &rom_contents)
    : cartridge(rom_contents),
      bus(cartridge, controller, communication, div_timer, sound, ppu, interrupt_state),
      pixel_buffer(LCD_WIDTH*LCD_HEIGHT)
{}

void Gameboy::print_cartridge_info() const {

    fmt::print("Title:             {}\n\n", this->cartridge.get_title());

    fmt::print("CGB flag:          ${:02X}\n\n", this->cartridge.get_cgb_flag());

    fmt::print("SGB flag:          ${:02X}\n\n", this->cartridge.get_sgb_flag());

    fmt::print("Cartridge type:    {}\n\n", this->cartridge.get_type_str());

    fmt::print("ROM size:          {} KiB, {} banks\n\n",
               this->cartridge.get_rom_size() / 1024,
               this->cartridge.get_rom_banks());

    fmt::print("RAM size code:     {} KiB, {} banks\n\n",
               this->cartridge.get_ram_size() / 1024,
               this->cartridge.get_ram_banks());

    const auto destination_code = this->cartridge.get_destination_code();
    fmt::print("Destination code:  ${:02X} ({})\n\n",
               destination_code,
               destination_code==0 ? "Japanese" : "Non-Japanese");

    fmt::print("Licensee code:     {}\n\n", this->cartridge.get_licensee_code());

    fmt::print("Mask ROM version:  ${:02X}\n\n", this->cartridge.get_mask_rom_version());
}

void Gameboy::reset() {
    this->cpu.reset();
    this->bus.write(0xFF05, 0x00); //   ; TIMA
    this->bus.write(0xFF06, 0x00); //   ; TMA
    this->bus.write(0xFF07, 0x00); //   ; TAC
    this->bus.write(0xFF10, 0x80); //   ; NR10
    this->bus.write(0xFF11, 0xBF); //   ; NR11
    this->bus.write(0xFF12, 0xF3); //   ; NR12
    this->bus.write(0xFF14, 0xBF); //   ; NR14
    this->bus.write(0xFF16, 0x3F); //   ; NR21
    this->bus.write(0xFF17, 0x00); //   ; NR22
    this->bus.write(0xFF19, 0xBF); //   ; NR24
    this->bus.write(0xFF1A, 0x7F); //   ; NR30
    this->bus.write(0xFF1B, 0xFF); //   ; NR31
    this->bus.write(0xFF1C, 0x9F); //   ; NR32
    this->bus.write(0xFF1E, 0xBF); //   ; NR33
    this->bus.write(0xFF20, 0xFF); //   ; NR41
    this->bus.write(0xFF21, 0x00); //   ; NR42
    this->bus.write(0xFF22, 0x00); //   ; NR43
    this->bus.write(0xFF23, 0xBF); //   ; NR44
    this->bus.write(0xFF24, 0x77); //   ; NR50
    this->bus.write(0xFF25, 0xF3); //   ; NR51
    this->bus.write(0xFF26, 0xF1); //($F0-SGB) ; NR52
    this->bus.write(0xFF40, 0x91); //   ; LCDC
    this->bus.write(0xFF42, 0x00); //   ; SCY
    this->bus.write(0xFF43, 0x00); //   ; SCX
    this->bus.write(0xFF45, 0x00); //   ; LYC
    this->bus.write(0xFF47, 0xFC); //   ; BGP
    this->bus.write(0xFF48, 0xFF); //   ; OBP0
    this->bus.write(0xFF49, 0xFF); //   ; OBP1
    this->bus.write(0xFF4A, 0x00); //   ; WY
    this->bus.write(0xFF4B, 0x00); //   ; WX
    this->bus.write(0xFFFF, 0x00); //   ; IE
}

void Gameboy::do_tick() {
    if(this->interrupt_state.get_interrupts()) {
        this->cpu.unhalt();
    }

    if(!this->cpu.is_halted()) {
        this->ppu.tick_dma(this->clock, bus);

        this->cpu.do_tick(this->clock, this->bus, this->interrupt_state);

        this->ppu.do_tick(this->pixel_buffer, this->bus, this->interrupt_state);
    }

    this->div_timer.do_tick(this->clock, this->interrupt_state);

    this->clock++;
}

void Gameboy::dump(std::ostream &os) const {
    os << "Registers:\n";
    cpu.dump(os);
    os << "\nMemory dump:\n";
    bus.dump(os);
}
