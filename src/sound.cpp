#include "sound.h"

#include <fmt/core.h>

void Sound::reset() {
    this->write_reg(0x10, 0x80); //   ; NR10
    this->write_reg(0x11, 0xBF); //   ; NR11
    this->write_reg(0x12, 0xF3); //   ; NR12
    this->write_reg(0x14, 0xBF); //   ; NR14
    this->write_reg(0x16, 0x3F); //   ; NR21
    this->write_reg(0x17, 0x00); //   ; NR22
    this->write_reg(0x19, 0xBF); //   ; NR24
    this->write_reg(0x1A, 0x7F); //   ; NR30
    this->write_reg(0x1B, 0xFF); //   ; NR31
    this->write_reg(0x1C, 0x9F); //   ; NR32
    this->write_reg(0x1E, 0xBF); //   ; NR33
    this->write_reg(0x20, 0xFF); //   ; NR41
    this->write_reg(0x21, 0x00); //   ; NR42
    this->write_reg(0x22, 0x00); //   ; NR43
    this->write_reg(0x23, 0xBF); //   ; NR44
    this->write_reg(0x24, 0x77); //   ; NR50
    this->write_reg(0x25, 0xF3); //   ; NR51
    this->write_reg(0x26, 0xF1); //($F0-SGB) ; NR52
}

uint8_t Sound::read_reg(uint8_t regid) const {
    switch (regid) {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:

        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:

        case 0x1a:
        case 0x1b:
        case 0x1c:
        case 0x1d:
        case 0x1e:

        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:

        case 0x24:
        case 0x25:
        case 0x26:

        // wave pattern ram
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
            return 0x00;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to Sound: {}", regid));
    }
}

void Sound::write_reg(uint8_t regid, uint8_t data) {
    switch (regid) {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:

        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:

        case 0x1a:
        case 0x1b:
        case 0x1c:
        case 0x1d:
        case 0x1e:

        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:

        case 0x24:
        case 0x25:
        case 0x26:

        // wave pattern ram
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
            break;
        default:
            throw std::runtime_error(fmt::format("Invalid regid passed to Sound: {}", regid));
    }
}

void Sound::dump(std::ostream &os) const {
    os << fmt::format("Sound not implemented...\n");
}
