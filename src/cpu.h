#ifndef CPU_H
#define CPU_H

#include "ibus.h"

#include <cstdint>
#include <optional>
#include <string>

class InterruptState;
enum class InterruptCause;

union reg {
    uint16_t r16;
    struct {
        uint8_t lo, hi;
    } r8;
};

class Cpu {
public:
    void reset();
    void do_tick(uint64_t clock, IBus &bus, InterruptState &int_state);
    void dump(std::ostream &os) const;

    bool is_halted() const {
        return this->halted;
    }

    void unhalt() {
        this->halted = false;
    }

private:
    void set_flags(bool z, bool n, bool h, bool c) {
        this->flag_z = z;
        this->flag_n = n;
        this->flag_h = h;
        this->flag_c = c;
    }

    uint8_t f() const {
        return (this->flag_z << 7) | (this->flag_n << 6) | (this->flag_h << 5) | (this->flag_c << 4);
    }

    void set_f(uint8_t v) {
        this->flag_z = v & 0x80;
        this->flag_n = v & 0x40;
        this->flag_h = v & 0x20;
        this->flag_c = v & 0x10;
    }

    uint8_t &decode_reg8(uint8_t bits);
    std::string decode_reg8_name(uint8_t bits) const;

    uint16_t &decode_reg16(uint8_t bits);
    std::string decode_reg16_name(uint8_t bits) const;

    uint16_t decode_stack_reg16_value(uint8_t bits) const;
    std::string decode_stack_reg16_name(uint8_t bits) const;

    void add(uint8_t val, bool with_carry);
    uint8_t get_sub(uint8_t val, bool with_carry);
    uint16_t get_add16(const uint16_t r1, const uint16_t r2);
    uint16_t get_add_sp(int8_t s8);

    uint8_t get_inc(uint8_t oldval);
    uint8_t get_dec(uint8_t oldval);

    void rlc(uint8_t &reg, bool with_z_flag);
    void rrc(uint8_t &reg, bool with_z_flag);
    void rl(uint8_t &reg, bool with_z_flag);
    void rr(uint8_t &reg, bool with_z_flag);

    // registers
    uint8_t a{0};
    reg bc{0};
    reg de{0};
    reg hl{0};
    uint16_t sp{0};
    uint16_t pc{0};
    bool flag_z{false};
    bool flag_n{false};
    bool flag_h{false};
    bool flag_c{false};
    bool ime{false};

    bool halted{false};
    std::optional<InterruptCause> isr_active;
    int cycle{0};
    uint8_t opcode{0};
    uint8_t tmp1, tmp2; // storage between cpu cycles
};

#endif /* CPU_H */
