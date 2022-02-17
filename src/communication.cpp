#include "communication.h"

#include <fmt/core.h>

// TODO: set interrupt (bit 3 in IF) appropriately

uint8_t Communication::read_reg(uint8_t regid) const
{
    switch(regid) {
    case 1:
        return this->sb;
    case 2:
        return this->sc;
    default:
        throw std::runtime_error(fmt::format("Invalid regid passed to Communication: {}", regid));
    }
}

void Communication::write_reg(uint8_t regid, uint8_t data) {
    switch(regid) {
    case 1:
        this->sb = data;
        break;
    case 2:
        this->sc = data;
        break;
    default:
        throw std::runtime_error(fmt::format("Invalid regid passed to Communication: {}", regid));
    }
}

void Communication::dump(std::ostream &os) const {
    os << fmt::format("SB [0xff01]: {:02X}\n", this->sb);
    os << fmt::format("SC [0xff02]: {:02X}\n", this->sc);
}
