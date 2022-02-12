#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <string>

class Bus;

class Cpu {
public:
    void reset();
    void do_tick(Bus &bus);

    bool get_flag_z() const {
        return flags & 0x80;
    }
    bool get_flag_n() const {
        return flags & 0x40;
    }
    bool get_flag_h() const {
        return flags & 0x20;
    }
    bool get_flag_c() const {
        return flags & 0x10;
    }

    void set_flag_z(bool val) {
        if(val) {
            flags |= 0x80;
        } else {
            flags &= ~0x80;
        }
    }
    void set_flag_n(bool val) {
        if(val) {
            flags |= 0x40;
        } else {
            flags &= ~0x40;
        }
    }
    void set_flag_h(bool val) {
        if(val) {
            flags |= 0x20;
        } else {
            flags &= ~0x20;
        }
    }
    void set_flag_c(bool val) {
        if(val) {
            flags |= 0x10;
        } else {
            flags &= ~0x10;
        }
    }

    uint8_t &decode_reg(uint8_t bits);
    std::string decode_reg_name(uint8_t bits) const;

private:

    // registers
    uint8_t a{0};
    uint8_t flags{0};
    uint8_t b{0}, c{0};
    uint8_t d{0}, e{0};
    uint8_t h{0}, l{0};
    uint16_t sp{0};
    uint16_t pc{0};
    bool ime{false};

    int cycle{0};
    uint8_t opcode{0};
    uint8_t tmp1, tmp2; // storage between cpu cycles
};

#endif /* CPU_H */
