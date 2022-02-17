#include "div_timer.h"

#include <fmt/core.h>

// TODO: set interrupt (bit 2 in IF) appropriately

uint8_t DivTimer::read_reg(uint8_t regid) const {
    fmt::print("================================================================================\n");
    fmt::print("   WARNING: READ AT ${:04X} (Div/Timer), but DivTimer is not implemented!\n", 0xff00|regid);
    fmt::print("=================================================================================\n");
    switch (regid) {
        case 4:
            return this->div;
        case 5:
            return this->tima;
        case 6:
            return this->tma;
        case 7:
            return this->tac;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to DivTimer: {}", regid));
    }
}

void DivTimer::write_reg(uint8_t regid, uint8_t data) {
    fmt::print("====================================================================================\n");
    fmt::print("   WARNING: WRITE AT ${:04X} (Div/Timer), data=${:02X}, but DivTimer is not implemented!\n", 0xff00|regid, data);
    fmt::print("====================================================================================\n");
    switch (regid) {
        case 4:
            this->div = data;
            break;
        case 5:
            this->tima = data;
            break;
        case 6:
            this->tma = data;
            break;
        case 7:
            this->tac = data;
            break;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to DivTimer: {}", regid));
    }
}

void DivTimer::dump(std::ostream &os) const {
    os << fmt::format("DIV  [0xff04]: {:02X}\n", this->div);
    os << fmt::format("TIMA [0xff05]: {:02X}\n", this->tima);
    os << fmt::format("TMA  [0xff06]: {:02X}\n", this->tma);
    os << fmt::format("TAC  [0xff07]: {:02X}\n", this->tac);
}
