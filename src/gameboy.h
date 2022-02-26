#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "ppu.h"
#include "controller.h"
#include "communication.h"
#include "sound.h"
#include "div_timer.h"
#include "interrupt_state.h"
#include "bus.h"

#include <cstdint>

class Gameboy {
public:
    Gameboy(std::vector<uint8_t> &rom_contents);

    void print_header() const;

    void reset();

    uint32_t *get_pixel_buffer_data() {
        return pixel_buffer.data();
    }

    void do_tick();
    void dump(std::ostream &os) const;
private:
    uint64_t clock{0};
    CartridgeType cartridge_type;
    size_t cartridge_ram_size;
    std::vector<uint8_t> &rom;
    Cpu cpu;
    Sound sound;
    Controller controller;
    Communication communication;
    DivTimer div_timer;
    InterruptState interrupt_state;
    Ppu ppu;
    Bus bus;
    std::vector<uint32_t> pixel_buffer;
};

#endif /* GAMEBOY_H */
