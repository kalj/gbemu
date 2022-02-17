#include "log.h"
#include "cpu.h"
#include "bus.h"

#include <fmt/core.h>

bool have_opcode(uint8_t op) {

    std::vector<uint8_t> rom;
    rom.push_back(op);

    log_set_enable(false);
    Cpu cpu;
    Bus bus{CartridgeType::ROM_ONLY, rom, 0};

    try {
        cpu.do_tick(bus);
        return true;
    } catch(...) {
        return false;
    }
}


int main() {

    for(int i=0; i<256; i++) {
        if(i%16 == 0) {
            fmt::print("\n");
        }

        if(have_opcode(i)) {
            fmt::print("   ");
        } else {
            fmt::print(" {:02X}", i);
        }
    }
    fmt::print("\n");

    return 0;
}
