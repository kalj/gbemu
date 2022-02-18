#include "log.h"
#include "gameboy.h"
#include "ppu.h"
#include "controller.h"
#include "communication.h"
#include "sound.h"
#include "div_timer.h"
#include "interrupt_state.h"
#include "bus.h"

#include <fmt/core.h>

bool test_valid_rom(const std::vector<uint8_t> &rom) {
    Cpu cpu;
    Sound snd;
    Controller cntl;
    Communication comm;
    DivTimer dt;
    InterruptState int_state;
    Ppu ppu;
    Bus bus{CartridgeType::ROM_ONLY, rom, 0, cntl, comm, dt, snd, ppu, int_state};

    try {
        cpu.do_tick(bus);
        cpu.do_tick(bus);
        return true;
    } catch(...) {
        return false;
    }
}

bool have_opcode(uint8_t op) {
    std::vector<uint8_t> rom;
    rom.push_back(op);

    return test_valid_rom(rom);
}

bool have_16bit_opcode(uint8_t op) {

    std::vector<uint8_t> rom;
    rom.push_back(0xcb);
    rom.push_back(op);

    return test_valid_rom(rom);
}

void print_matrix(const std::vector<bool> &results) {
    fmt::print("    ");
    for(int i=0; i<16; i++) {
        fmt::print(" x{:X}",i);
    }
    fmt::print("\n    -------------------------------------------------");
    for(int i=0; i<256; i++) {
        if(i%16 == 0) {
            fmt::print("\n {0:X}x|",i/16);
        }

        if(results[i]) {
            fmt::print("   ");
        } else {
            fmt::print(" {:02X}", i);
        }
    }
    fmt::print("\n");
}

int main() {
    log_set_enable(false);

    std::vector<bool> results(256);
    for(int i=0; i<256; i++) {
        results[i] = have_opcode(i);
    }
    fmt::print("8-bit instructions\n\n");
    print_matrix(results);

    for(int i=0; i<256; i++) {
        results[i] = have_16bit_opcode(i);
    }
    fmt::print("\n16-bit instructions\n\n");
    print_matrix(results);

    return 0;
}
