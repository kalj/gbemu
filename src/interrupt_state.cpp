#include "interrupt_state.h"

#include <fmt/core.h>

void InterruptState::set_if_vblank() {
    fmt::print("#######################################\n");
    fmt::print("        Setting IF bit for VBlank      \n");
    fmt::print("#######################################\n");
    this->if_reg |= 0x01;
}

void InterruptState::set_if_stat() {
    fmt::print("#######################################\n");
    fmt::print("       Setting IF bit for LCD STAT     \n");
    fmt::print("#######################################\n");
    this->if_reg |= 0x02;
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
