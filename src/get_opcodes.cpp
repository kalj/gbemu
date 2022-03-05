#include "logging.h"
#include "cpu.h"
#include "interrupt_state.h"
#include "bus.h"

#include <fmt/core.h>

class MockBus : public IBus {
public:
    MockBus(const std::vector<uint8_t> &rom) : rom(rom) {}

    uint8_t read(uint16_t addr) const override {
        return rom[addr];
    }

    void write(uint16_t addr, uint8_t data) override {
        // do nothing
    }

private:
    const std::vector<uint8_t> &rom;
};

bool test_valid_rom(const std::vector<uint8_t> &rom) {
    Cpu cpu;
    InterruptState int_state;
    MockBus bus{rom};

    try {
        for(size_t i=0; i<4*rom.size(); i++) {
            cpu.do_tick(i, bus, int_state);
        }
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
    logging::set_level(logging::LogLevel::QUIET);

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
