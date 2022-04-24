#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "bus.h"
#include "cartridge.h"
#include "communication.h"
#include "controller.h"
#include "cpu.h"
#include "div_timer.h"
#include "interrupt_state.h"
#include "ppu.h"
#include "sound.h"

#include <cstdint>

class Gameboy {
public:
    Gameboy(const std::vector<uint8_t> &rom_contents);

    void print_cartridge_info() const;

    void reset();

    uint32_t *get_pixel_buffer_data() {
        return pixel_buffer.data();
    }

    void do_tick();

    void set_button_state(gb_controller::Button button, gb_controller::State state) {
        this->controller.set_button_state(button, state);
    }

    void render_audio(int16_t *buffer, int n_frames) {
        this->sound.render(buffer, n_frames);
    }

    void dump(std::ostream &os) const;

private:
    uint64_t clock{0};
    Cartridge cartridge;
    Cpu cpu;
    Sound sound;
    gb_controller::Controller controller;
    Communication communication;
    DivTimer div_timer;
    InterruptState interrupt_state;
    Ppu ppu;
    Bus bus;
    std::vector<uint32_t> pixel_buffer;
};

#endif /* GAMEBOY_H */
