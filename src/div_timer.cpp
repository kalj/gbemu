#include "div_timer.h"
#include "interrupt_state.h"

#include <array>
#include <fmt/core.h>

const static std::array<uint16_t,4> divisors = { 1024, 16, 64, 256 };

void DivTimer::reset() {
    this->write_reg(0x05, 0x00); //   ; TIMA
    this->write_reg(0x06, 0x00); //   ; TMA
    this->write_reg(0x07, 0x00); //   ; TAC
}

void DivTimer::do_tick(uint64_t clock, InterruptState &int_state) {
    if(clock%256 == 0) {
        this->div++;
    }

    if(this->timer_enable &&
       (clock%divisors[this->clock_select] == 0)) {
        this->timer++;
        if(this->timer == 0) {
            this->timer = this->timer_modulo;
            // set interrupt
            int_state.set_if_bit(InterruptCause::TIMER);
        }
    }
}

uint8_t DivTimer::read_reg(uint8_t regid) const {
    switch (regid) {
        case 4:
            return this->div;
        case 5:
            return this->timer;
        case 6:
            return this->timer_modulo;
        case 7:
            return (this->timer_enable?0x04:0)|this->clock_select;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to DivTimer: {}", regid));
    }
}

void DivTimer::write_reg(uint8_t regid, uint8_t data) {
    switch (regid) {
        case 4:
            this->div = 0;
            break;
        case 5:
            this->timer = data;
            break;
        case 6:
            this->timer_modulo = data;
            break;
        case 7:
            this->timer_enable = data&0x04;
            this->clock_select = data&0x03;
            break;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to DivTimer: {}", regid));
    }
}

void DivTimer::dump(std::ostream &os) const {
    os << fmt::format("DivTimer state:\n");
    os << fmt::format("  DIV  [0xFF04]: {:02X}\n", this->div);
    os << fmt::format("  TIMA [0xFF05]: {:02X}\n", this->timer);
    os << fmt::format("  TMA  [0xFF06]: {:02X}\n", this->timer_modulo);
    os << fmt::format("  TAC  [0xFF07]: {:02X}\n", (this->timer_enable?0x04:0)|this->clock_select);
}
