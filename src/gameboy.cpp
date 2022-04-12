#include "gameboy.h"

#include <fmt/core.h>

Gameboy::Gameboy(const std::vector<uint8_t> &rom_contents)
    : cartridge(rom_contents),
      bus(cartridge, controller, communication, div_timer, sound, ppu, interrupt_state),
      pixel_buffer(LCD_WIDTH * LCD_HEIGHT) {
}

void Gameboy::print_cartridge_info() const {

    fmt::print("Title:             {}\n", this->cartridge.get_title());

    fmt::print("CGB flag:          ${:02X}\n", this->cartridge.get_cgb_flag());

    fmt::print("SGB flag:          ${:02X}\n", this->cartridge.get_sgb_flag());

    fmt::print("Cartridge type:    {}\n", this->cartridge.get_type_str());

    fmt::print("ROM size:          {} KiB, {} banks\n",
               this->cartridge.get_rom_size() / 1024,
               this->cartridge.get_rom_banks());

    fmt::print("RAM size code:     {} KiB, {} banks\n",
               this->cartridge.get_ram_size() / 1024,
               this->cartridge.get_ram_banks());

    const auto destination_code = this->cartridge.get_destination_code();
    fmt::print("Destination code:  ${:02X} ({})\n",
               destination_code,
               destination_code == 0 ? "Japanese" : "Non-Japanese");

    fmt::print("Licensee code:     {}\n", this->cartridge.get_licensee_code());

    fmt::print("Mask ROM version:  ${:02X}\n", this->cartridge.get_mask_rom_version());
}

void Gameboy::reset() {
    this->cpu.reset();
    this->div_timer.reset();
    this->sound.reset();
    this->ppu.reset();
    this->interrupt_state.reset();
}

void Gameboy::do_tick() {
    if (this->interrupt_state.get_interrupts()) {
        this->cpu.unhalt();
    }

    this->ppu.tick_dma(this->clock, bus);

    if (!this->cpu.is_halted()) {
        this->cpu.do_tick(this->clock, this->bus, this->interrupt_state);
    }

    this->ppu.do_tick(this->pixel_buffer, this->bus, this->interrupt_state);

    this->div_timer.do_tick(this->clock, this->interrupt_state);

    this->clock++;
}

void Gameboy::dump(std::ostream &os) const {
    os << "Registers:\n";
    cpu.dump(os);
    os << "\nMemory dump:\n";
    bus.dump(os);
}
