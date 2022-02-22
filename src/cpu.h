#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <string>
#include <optional>

class Bus;
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
    void do_tick(Bus &bus, InterruptState &int_state);
    void dump(std::ostream &os) const;

private:
    uint8_t &a() {
        return this->af.r8.hi;
    }

    uint8_t &flags() {
        return this->af.r8.lo;
    }

    const uint8_t &flags() const {
        return this->af.r8.lo;
    }

    bool get_flag_z() const {
        return this->flags() & 0x80;
    }
    bool get_flag_n() const {
        return this->flags() & 0x40;
    }
    bool get_flag_h() const {
        return this->flags() & 0x20;
    }
    bool get_flag_c() const {
        return this->flags() & 0x10;
    }

    void set_flag_z(bool val) {
        if(val) {
            this->flags() |= 0x80;
        } else {
            this->flags() &= ~0x80;
        }
    }
    void set_flag_n(bool val) {
        if(val) {
            this->flags() |= 0x40;
        } else {
            this->flags() &= ~0x40;
        }
    }
    void set_flag_h(bool val) {
        if(val) {
            this->flags() |= 0x20;
        } else {
            this->flags() &= ~0x20;
        }
    }
    void set_flag_c(bool val) {
        if(val) {
            this->flags() |= 0x10;
        } else {
            this->flags() &= ~0x10;
        }
    }

    uint8_t &decode_reg8(uint8_t bits);
    std::string decode_reg8_name(uint8_t bits) const;

    uint16_t &decode_reg16(uint8_t bits);
    std::string decode_reg16_name(uint8_t bits) const;

    uint16_t &decode_stack_reg16(uint8_t bits);
    std::string decode_stack_reg16_name(uint8_t bits) const;

    void add(uint8_t val, bool with_carry);
    uint8_t get_sub(uint8_t val, bool with_carry);
    uint16_t get_add16(const uint16_t r1, const uint16_t r2);

    uint8_t get_inc(uint8_t oldval);
    uint8_t get_dec(uint8_t oldval);

    void rlc(uint8_t &reg, bool with_z_flag);
    void rrc(uint8_t &reg, bool with_z_flag);
    void rl(uint8_t &reg, bool with_z_flag);
    void rr(uint8_t &reg, bool with_z_flag);

    // registers
    reg af{0};
    reg bc{0};
    reg de{0};
    reg hl{0};
    uint16_t sp{0};
    uint16_t pc{0};
    bool ime{false};

    std::optional<InterruptCause> isr_active;
    int cycle{0};
    uint8_t opcode{0};
    uint8_t tmp1, tmp2; // storage between cpu cycles
};

#endif /* CPU_H */
