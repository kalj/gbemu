#ifndef CPU_H
#define CPU_H

#include <cstdint>

class Bus;

class Cpu {
public:
    void reset();
    void do_instruction(Bus &bus);
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

private:
    uint8_t a{0};
    uint8_t flags{0};
    uint8_t b{0}, c{0};
    uint8_t d{0}, e{0};
    uint8_t h{0}, l{0};
    uint16_t sp{0};
    uint16_t pc{0};
    bool ime{false};
};

#endif /* CPU_H */
