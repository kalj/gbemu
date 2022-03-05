#include "interrupt_state.h"

#include <fmt/core.h>

std::string interrupt_cause_to_string(InterruptCause ic) {
    using enum InterruptCause;
    switch(ic) {
    case VBLANK:
        return "VBlank";
    case LCD_STAT:
        return "LCD STAT";
    case TIMER:
        return "Timer";
    case SERIAL:
        return "Serial";
    case JOYPAD:
        return "Joypad";
    }
    throw std::runtime_error("Shouldn't ever end up here!");
}

void InterruptState::reset() {
    this->write_reg(0xFF, 0x00); //   ; IE
}

void InterruptState::set_if_bit(InterruptCause ic) {
    this->if_reg |= 1<<static_cast<uint8_t>(ic);
}

void InterruptState::clear_if_bit(InterruptCause ic) {
    this->if_reg &= ~(1<<static_cast<uint8_t>(ic));
}

uint8_t InterruptState::read_reg(uint8_t regid) const {
    switch (regid) {
        case 0x0f:
            return this->if_reg;
        case 0xff:
            return this->ie_reg;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to InterruptState: ${:02X}", regid));
    }
}

void InterruptState::write_reg(uint8_t regid, uint8_t data) {
    switch (regid) {
        case 0x0f:
            this->if_reg = data;
            break;
        case 0xff:
            this->ie_reg = data;
            break;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to InterruptState: ${:02X}", regid));
    }
}

void InterruptState::dump(std::ostream &os) const {
    os << fmt::format("IF [0xff0f]: {:02X}\n", this->if_reg);
    os << fmt::format("IE [0xffff]: {:02X}\n", this->ie_reg);
}
