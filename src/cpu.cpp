#include "cpu.h"

#include "interrupt_state.h"
#include "log.h"
#include "bus.h"

#include <fmt/core.h>

void Cpu::reset() {
    this->af.r8.hi = 0x01; // on GB/SGB. 0xff on GBP, 0x11 on GBC
    this->af.r8.lo = 0xb0;
    this->bc.r16 = 0x0013; // on GB/SGB/GBP/GBC
    this->de.r16 = 0x00d8;
    this->hl.r16 = 0x014d;
    this->sp    = 0xfffe;
    this->pc    = 0x0100;

    this->cycle = 0;
    this->ime = false;
}

uint8_t &Cpu::decode_reg8(uint8_t bits) {
    switch(bits) {
    case 0:
        return this->bc.r8.hi;
    case 1:
        return this->bc.r8.lo;
    case 2:
        return this->de.r8.hi;
    case 3:
        return this->de.r8.lo;
    case 4:
        return this->hl.r8.hi;
    case 5:
        return this->hl.r8.lo;
    default: // 7
        return this->af.r8.hi;
    }
}

std::string Cpu::decode_reg8_name(uint8_t bits) const {
    switch(bits) {
    case 0:
        return "B";
    case 1:
        return "C";
    case 2:
        return "D";
    case 3:
        return "E";
    case 4:
        return "H";
    case 5:
        return "L";
    default: // 7
        return "A";
    }
}

uint16_t &Cpu::decode_reg16(uint8_t bits) {
    switch(bits) {
    case 0:
        return this->bc.r16;
    case 1:
        return this->de.r16;
    case 2:
        return this->hl.r16;
    default: // 3
        return this->sp;
    }
}

std::string Cpu::decode_reg16_name(uint8_t bits) const {
    switch(bits) {
    case 0:
        return "BC";
    case 1:
        return "DE";
    case 2:
        return "HL";
    default: // 3
        return "SP";
    }
}

uint16_t &Cpu::decode_stack_reg16(uint8_t bits) {
    switch(bits) {
    case 0:
        return this->bc.r16;
    case 1:
        return this->de.r16;
    case 2:
        return this->hl.r16;
    default: // 3
        return this->af.r16;
    }
}

std::string Cpu::decode_stack_reg16_name(uint8_t bits) const {
    switch(bits) {
    case 0:
        return "BC";
    case 1:
        return "DE";
    case 2:
        return "HL";
    default: // 3
        return "AF";
    }
}

void Cpu::add(uint8_t val, bool with_carry) {

    const uint8_t res5  = (this->a() & 0xf) + (val & 0xf) + (with_carry && this->get_flag_c() ? 1 : 0);
    const uint16_t res9 = static_cast<uint16_t>(this->a()) + val + (with_carry && this->get_flag_c() ? 1 : 0);
    this->a() = res9 & 0xff;

    this->set_flag_h(res5 & 0x10);
    this->set_flag_c(res9&0x100);
    this->set_flag_z(this->a() == 0);
    this->set_flag_n(false);
}

uint8_t Cpu::get_sub(uint8_t val, bool with_carry) {
    const uint8_t res5 = (this->a()&0xf) - (val&0xf) -(with_carry&&this->get_flag_c()?1:0);
    const uint16_t res9 = static_cast<uint16_t>(this->a()) - val -(with_carry&&this->get_flag_c()?1:0);

    const auto res = res9&0xff;
    this->set_flag_h(res5&0x10);
    this->set_flag_c(res9&0x100);
    this->set_flag_n(true);
    this->set_flag_z(res == 0);

    return res;
}

uint8_t Cpu::get_inc(uint8_t oldval) {
    const uint8_t res5 = (oldval & 0xf) + 1;
    const uint8_t newval  = oldval + 1;

    this->set_flag_h(res5 & 0x10);
    this->set_flag_z(newval == 0);
    this->set_flag_n(false);

    return newval;
}

uint8_t Cpu::get_dec(uint8_t oldval) {
    const uint8_t res5 = (oldval & 0xf) - 1;
    const uint8_t newval  = oldval - 1;

    this->set_flag_h(res5 & 0x10);
    this->set_flag_z(newval == 0);
    this->set_flag_n(true);

    return newval;
}

void Cpu::rlc(uint8_t &reg, bool with_z_flag) {
    this->set_flag_c(reg & 0x80);           // bit 7 goes into carry
    reg = (reg << 1) | ((reg & 0x80) >> 7); // rotate left without carry
    this->set_flag_h(false);
    this->set_flag_n(false);
    this->set_flag_z(with_z_flag && reg == 0);
}

void Cpu::rrc(uint8_t &reg, bool with_z_flag) {
    this->set_flag_c(reg & 0x01);           // bit 0 goes into carry
    reg = ((reg & 0x01) << 7) | (reg >> 1); // rotate right without carry
    this->set_flag_z(with_z_flag && reg == 0);
    this->set_flag_h(false);
    this->set_flag_n(false);
}

void Cpu::rl(uint8_t &reg, bool with_z_flag) {
    const uint8_t res = (reg << 1) | (this->get_flag_c() ? 0x01 : 0x00); // rotate left with carry.
    this->set_flag_c(reg & 0x80);                                        // bit 7 goes into carry
    reg = res;
    this->set_flag_z(with_z_flag && reg == 0);
    this->set_flag_h(false);
    this->set_flag_n(false);
}

void Cpu::rr(uint8_t &reg, bool with_z_flag) {
    const uint8_t res = (this->get_flag_c() ? 0x80 : 0x00) | (reg >> 1); // rotate right with carry;
    this->set_flag_c(reg & 0x01);                                        // bit 0 goes into carry
    reg = res;
    this->set_flag_z(with_z_flag && reg == 0);
    this->set_flag_h(false);
    this->set_flag_n(false);
}

void Cpu::do_tick(Bus &bus, InterruptState &int_state) {

    if(this->cycle == 0) {
        // check for interrupts:
        const auto interrupts = int_state.get_interrupts();
        if (this->ime && interrupts) {
            this->isr_active = interrupts & (1 << 0)   ? InterruptCause::VBLANK
                               : interrupts & (1 << 1) ? InterruptCause::LCD_STAT
                               : interrupts & (1 << 2) ? InterruptCause::TIMER
                               : interrupts & (1 << 3) ? InterruptCause::SERIAL
                                                       : InterruptCause::JOYPAD; // interrupts&(1<<4)?

            this->cycle++;
            log(fmt::format("\t\t\t\t\t\t\t\t Interrupt detected: {}\n", interrupt_cause_to_string(this->isr_active.value())));
        } else {
            this->opcode = bus.read(this->pc);
            log(fmt::format("\t\t\t\t\t\t\t\t read opcode: ${:02X} -> ", this->opcode));
        }
    }

    if(this->isr_active) {
        if(this->cycle==1) {
            this->ime = false;
            int_state.clear_if_bit(this->isr_active.value());
            this->cycle++;
        } else if(this->cycle == 2) {
            // push PC MSB
            this->sp--;
            bus.write(this->sp, (this->pc >> 8)&0xff);
            this->cycle++;
        } else if(this->cycle == 3) {
            // push PC LSB
            this->sp--;
            bus.write(this->sp, this->pc&0xff);
            this->cycle++;
        } else if(this->cycle == 4) {
            // set PC to $40/$48/$50/$58/$60
            this->pc = 0x40 + 8*static_cast<uint8_t>(this->isr_active.value());
            log(fmt::format("\t\t\t\t\t\t\t\t Jumping to interrupt handler at ${:04X}\n", this->pc));
            this->isr_active.reset();
            this->cycle = 0;
        }

        return;
    }

    switch (this->opcode) {
        case 0x00:
            log(fmt::format("NOP\n"));
            this->pc += 1;
            this->cycle = 0; // start over
            break;

        case 0x2F: { // Complement A
            log(fmt::format("CPL\n"));
            this->a() = ~(this->a());
            this->set_flag_h(true);
            this->set_flag_n(true);
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = ~A = ${:02X}\n", this->a()));
            this->pc += 1;
        } break;

        case 0x37: { // Set carry flag
            log(fmt::format("SCF\n"));
            this->set_flag_c(true);
            this->set_flag_h(false);
            this->set_flag_n(false);
            log(fmt::format("\t\t\t\t\t\t\t\t\t Setting the carry flag\n"));
            this->pc++;
        } break;

        case 0x3F: { // Flip ("complement") carry flag
            log(fmt::format("CCF\n"));
            this->set_flag_c(!this->get_flag_c());
            this->set_flag_h(false);
            this->set_flag_n(false);
            log(fmt::format("\t\t\t\t\t\t\t\t\t Flipping the carry flag, new value: {}\n", this->get_flag_c()));
            this->pc++;
        } break;

        //-------------------------------------------------------
        // LD
        //-------------------------------------------------------

        // LD r1, r2

        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x47:

        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4f:

        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x57:

        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5f:

        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x67:

        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6f:

        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7f:
            {
                const auto src_name = decode_reg8_name(this->opcode & 0x7);
                const auto dst_name = decode_reg8_name((this->opcode >> 3) & 0x7);
                const auto &src           = decode_reg8(this->opcode & 0x7);
                auto &dst           = decode_reg8((this->opcode >> 3) & 0x7);
                log(fmt::format("LD {}, {}\n", dst_name, src_name));
                dst = src;
                log(fmt::format("\t\t\t\t\t\t\t\t\t {} loaded from {} (${:02X})\n", dst_name, src_name, dst));
                this->pc += 1;
            }
            break;
        case 0x46:
        case 0x4E:
        case 0x56:
        case 0x5E:
        case 0x66:
        case 0x6E:
        case 0x7E:
            if (this->cycle == 0) {
                const auto reg_name = decode_reg8_name((this->opcode >> 3) & 0x7);
                log(fmt::format("LD {}, (HL)\n", reg_name));
                this->cycle++;
            } else {
                const auto reg_name = decode_reg8_name((this->opcode >> 3) & 0x7);
                auto &reg           = decode_reg8((this->opcode >> 3) & 0x7);

                reg                 = bus.read(this->hl.r16);
                log(fmt::format("\t\t\t\t\t\t\t\t\t {} loaded from (HL) (${:04X}) = ${:02X}\n", reg_name, this->hl.r16, reg));
                this->pc += 1;
                this->cycle = 0;
            }
        break;

        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x77:
            if (this->cycle == 0) {
                const auto reg_name = decode_reg8_name(this->opcode & 0x7);
                log(fmt::format("LD (HL), {}\n", reg_name));
                this->cycle++;
            } else {
                const auto reg_name = decode_reg8_name(this->opcode & 0x7);
                const auto &reg     = decode_reg8(this->opcode & 0x7);

                bus.write(this->hl.r16, reg);
                log(fmt::format("\t\t\t\t\t\t\t\t\t {} written to (HL) (${:04X}) = ${:02X}\n",
                                reg_name,
                                this->hl.r16,
                                reg));
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        case 0x06: // LD B, d8    0000 0110
        case 0x0E: // LD C, d8    0000 1110
        case 0x16: // LD D, d8    0001 0110
        case 0x1E: // LD E, d8    0001 1110
        case 0x26: // LD H, d8    0010 0110
        case 0x2E: // LD L, d8    0010 1110
        case 0x3E: // LD A, d8    0011 1110
            if (this->cycle == 0) {
                const auto reg_name = decode_reg8_name((this->opcode >> 3) & 0x7);
                log(fmt::format("LD {}, d8\n", reg_name));
                this->cycle++;
            } else {
                const auto reg_name = decode_reg8_name((this->opcode >> 3) & 0x7);
                auto &reg           = decode_reg8((this->opcode >> 3) & 0x7);
                reg                 = bus.read(this->pc + 1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t {} set to ${:02X}\n", reg_name, reg));
                this->pc += 2;
                this->cycle = 0;
            }
            break;

        case 0x36: // LD (HL), d8
        {
            if (this->cycle == 0) {
                log(fmt::format("LD (HL), d8\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                bus.write(this->hl.r16, this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t ${:02X} stored to (hl) (${:04X})\n",
                                this->tmp1,
                                this->hl.r16));

                this->pc += 2;
                this->cycle = 0;
            }
        } break;

        case 0x0A: // LD A, (BC)
        case 0x1A: // LD A, (DE)
            if (this->cycle == 0) {
                const auto reg_name = this->opcode & 0x10 ? "DE" : "BC";
                log(fmt::format("LD A, ({})\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {

                const auto reg_name = this->opcode & 0x10 ? "DE" : "BC";
                const auto reg      = this->opcode & 0x10 ? this->de.r16 : this->bc.r16;
                this->a()           = bus.read(reg);
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) read from ({}) (${:04X})\n", this->a(), reg_name, reg));
                this->pc += 1;
                this->cycle = 0;
            }
            break;
        case 0x2A: // LD A, (HL+)
            if (this->cycle == 0) {
                log(fmt::format("LD A, (HL+)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->a() = bus.read(this->hl.r16);
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) read from (hl) (${:04X}), and hl incremented\n",
                                this->a(),
                                this->hl.r16));
                this->hl.r16++;
                this->pc += 1;
                this->cycle = 0;
            }
            break;
        case 0x3A: // LD A, (HL-)
            if (this->cycle == 0) {
                log(fmt::format("LD A, (HL-)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->a() = bus.read(this->hl.r16);
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) read from (hl) (${:04X}), and hl decremented\n",
                                this->a(),
                                this->hl.r16));
                this->hl.r16--;
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        case 0x02: // LD (BC), A
        case 0x12: // LD (DE), A
            if (this->cycle == 0) {
                const auto reg_name = this->opcode & 0x10 ? "DE" : "BC";
                log(fmt::format("LD ({}), A\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {
                const auto reg_name = this->opcode & 0x10 ? "DE" : "BC";
                const auto reg      = this->opcode & 0x10 ? this->de.r16 : this->bc.r16;

                bus.write(reg, this->a());
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to ({}) (${:04X})\n",
                                this->a(),
                                reg_name,
                                reg));
                this->pc += 1;
                this->cycle = 0;
            }
            break;
        case 0x22:
            if (this->cycle == 0) {
                log(fmt::format("LD (HL+), A\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                bus.write(this->hl.r16, this->a());
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (hl) (${:04X}), and hl incremented\n",
                                this->a(),
                                this->hl.r16));
                this->hl.r16++;
                this->pc += 1;
                this->cycle = 0;
            }
            break;
        case 0x32:
            if (this->cycle == 0) {
                log(fmt::format("LD (HL-), A\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                bus.write(this->hl.r16, this->a());
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (hl) (${:04X}), and hl decremented\n",
                                this->a(),
                                this->hl.r16));
                this->hl.r16--;
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        // 16-bit register load
        case 0x01: // LD BC, d16
        case 0x11: // LD DE, d16
        case 0x21: // LD HL, d16
        case 0x31: // LD SP, d16
            if (this->cycle == 0) {
                const auto reg_name = decode_reg16_name((this->opcode>>4)&0x3);
                log(fmt::format("LD {}, d16\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                const auto reg_name = decode_reg16_name((this->opcode>>4)&0x3);
                auto &reg           = decode_reg16((this->opcode>>4)&0x3);

                reg = static_cast<uint16_t>(bus.read(this->pc + 2))<<8 | this->tmp1;

                log(fmt::format("\t\t\t\t\t\t\t\t\t {} set to ${:04X}\n", reg_name, reg));
                this->pc += 3;
                this->cycle = 0;
            }
            break;
        case 0xF9: // LD SP, HL
            if (this->cycle == 0) {
                log(fmt::format("LD SP, HL\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->sp = this->hl.r16;
                log(fmt::format("\t\t\t\t\t\t\t\t\t SP set to HL: ${:04X}\n", this->sp));
                this->pc++;
                this->cycle = 0;
            }
            break;

        // store A to immediate address
        case 0xEA:
            if (this->cycle == 0) {
                log(fmt::format("LD (a16), A\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2);
                this->cycle++;
            } else if (this->cycle == 3) {
                const uint16_t addr = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                bus.write(addr, this->a());
                log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) stored to ${:04X}\n",
                                this->a(),
                                addr));
                this->pc += 3;
                this->cycle = 0;
            }
            break;
        // load A from immediate address
        case 0xFA:
            if (this->cycle == 0) {
                log(fmt::format("LD A, (a16)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2);
                this->cycle++;
            } else if (this->cycle == 3) {
                const uint16_t addr = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                this->a() = bus.read(addr);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A loaded from ${:04X} (${:02X})\n",
                                addr,
                                this->a()));
                this->pc += 3;
                this->cycle = 0;
            }
            break;

        // IO/HRAM PAGE LD instructions
        case 0xE0:
            if (this->cycle == 0) {
                log(fmt::format("LD (a8), A\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                const uint16_t addr = 0xff00 | this->tmp1;
                log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) stored to (${:04X})\n", this->a(), addr));
                bus.write(addr, this->a());
                this->pc += 2;
                this->cycle = 0;
            }
            break;
        case 0xF0:
            if (this->cycle == 0) {
                log(fmt::format("LD A, (a8)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                const uint16_t addr = 0xff00 | this->tmp1;
                this->a()           = bus.read(addr);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) loaded from (${:04X})\n", this->a(), addr));
                this->pc += 2;
                this->cycle = 0;
            }
            break;
        case 0xE2:
            if (this->cycle == 0) {
                log(fmt::format("LD (C), A\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                const uint16_t addr = 0xff00 | this->bc.r8.lo; // (0xff00|c)
                log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) stored to (${:04X})\n", this->a(), addr));
                bus.write(addr, this->a());
                this->pc += 1;
                this->cycle = 0;
            }
            break;
        case 0xF2:
            if (this->cycle == 0) {
                log(fmt::format("LD A, (C)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                const uint16_t addr = 0xff00 | this->bc.r8.lo; // (0xff00|c)
                this->a()           = bus.read(addr);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) loaded from (${:04X})\n", this->a(), addr));
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        // store SP to immediate address
        case 0x08:
            if (this->cycle == 0) {
                log(fmt::format("LD (a16), SP\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2);
                this->cycle++;
            } else if (this->cycle == 3) {
                const uint16_t addr = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                bus.write(addr, this->sp & 0xff);
                this->cycle++;
            } else if (this->cycle == 4) {
                const uint16_t addr = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                bus.write(addr + 1, (this->sp >> 8) & 0xff);
                log(fmt::format("\t\t\t\t\t\t\t\t\t SP (${:04X}) stored to ${:04X}\n", this->sp, addr));
                this->pc += 3;
                this->cycle = 0;
            }
            break;

        //-------------------------------------------------------
        // stack operations
        //-------------------------------------------------------

        case 0xC1: // POP
        case 0xD1:
        case 0xE1:
        case 0xF1: {
            const auto reg_name = decode_stack_reg16_name((this->opcode >> 4) & 0x3);
            auto &reg           = decode_stack_reg16((this->opcode >> 4) & 0x3);
            if (this->cycle == 0) {
                log(fmt::format("POP {}\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->sp); // LSB
                this->sp++;
                this->cycle++;
            } else {
                this->tmp2 = bus.read(this->sp); // MSB
                this->sp++;
                reg = static_cast<uint16_t>(this->tmp2) << 8 | this->tmp1;
                log(fmt::format("\t\t\t\t\t\t\t\t\t POP {} from stack: ${:04X}\n", reg_name, reg));
                this->pc += 1;
                this->cycle = 0;
            }
        } break;

        case 0xC5: // PUSH
        case 0xD5:
        case 0xE5:
        case 0xF5: {
            const auto reg_name = decode_stack_reg16_name((this->opcode >> 4) & 0x3);
            auto &reg           = decode_stack_reg16((this->opcode >> 4) & 0x3);
            if (this->cycle == 0) {
                log(fmt::format("PUSH {}\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->sp--;
                bus.write(this->sp, 0xff & (reg >> 8));
                this->cycle++;
            } else if (this->cycle == 2) {
                this->sp--;
                bus.write(this->sp, reg & 0xff);
                this->cycle++;
            } else {
                log(fmt::format("\t\t\t\t\t\t\t\t\t pushed {} onto stack: ${:04X}\n", reg_name, reg));
                this->pc += 1;
                this->cycle = 0;
            }
        } break;

            //-------------------------------------------------------
            // ALU
            //-------------------------------------------------------
        case 0x07: // RLCA
        {
            log(fmt::format("RLCA\n"));
            this->rlc(this->a(), false);

            log(fmt::format("\t\t\t\t\t\t\t\t\t 8-bit rotate left of A = ${:02X}\n", this->a()));
            this->pc += 1;
        } break;
        case 0x0F: // RRCA
        {
            log(fmt::format("RRCA\n"));
            this->rrc(this->a(), false);

            log(fmt::format("\t\t\t\t\t\t\t\t\t 8-bit rotate right of A = ${:02X}\n", this->a()));
            this->pc += 1;
        } break;
        case 0x17: // RLA
        {
            log(fmt::format("RLA\n"));
            this->rl(this->a(), false);

            log(fmt::format("\t\t\t\t\t\t\t\t\t 9-bit rotate left of A = ${:02X}\n", this->a()));
            this->pc += 1;
        } break;
        case 0x1F: // RRA
        {
            log(fmt::format("RRA\n"));
            this->rr(this->a(), false);

            log(fmt::format("\t\t\t\t\t\t\t\t\t 9-bit rotate right of A = ${:02X}\n", this->a()));
            this->pc += 1;
        } break;

        case 0x80: // ADD
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x87:

        case 0x88: // ADC
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        case 0x8d:
        case 0x8f:
        {
            const bool with_carry = this->opcode & 0x08;
            const auto instr_name = with_carry ? "ADC" : "ADD";
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);

            log(fmt::format("{} A, {}\n", instr_name, reg_name));

            const auto &reg           = decode_reg8(this->opcode & 0x7);
            this->add(reg, with_carry);

            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A {} {} = ${:02X}\n", instr_name, reg_name, this->a()));
            this->pc += 1;
        } break;

        case 0x86: // ADD A, (HL)
        case 0x8E: // ADC A, (HL)
        case 0xC6: // ADD A, d8
        case 0xCE: // ADC A, d8
            if (this->cycle == 0) {
                const bool with_carry   = this->opcode & 0x08;
                const auto instr_name   = with_carry ? "ADC" : "ADD";
                const auto operand_name = this->opcode & 0x40 ? "d8" : "(HL)";

                log(fmt::format("{} A, {}\n", instr_name, operand_name));
                this->cycle++;
                this->pc++;
            } else if (this->cycle == 1) {
                const bool with_carry      = this->opcode & 0x08;
                const auto instr_name      = with_carry ? "ADC" : "ADD";
                const auto operand_address = this->opcode & 0x40 ? this->pc++ : this->hl.r16;
                const auto operand         = bus.read(operand_address);
                this->add(operand, with_carry);

                const auto operand_str = this->opcode & 0x40
                                             ? fmt::format("${:02X}", operand)
                                             : fmt::format("${:02X} (@ ${:04X})", operand, operand_address);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A = A {} {} = ${:02X}\n", instr_name, operand_str, this->a()));
                this->cycle = 0;
            }
            break;

        case 0x96: // SUB A, (HL)
        case 0x9E: // SBC A, (HL)
        case 0xD6: // SUB A, d8
        case 0xDE: // SBC A, d8
            if (this->cycle == 0) {
                const bool with_carry   = this->opcode & 0x08;
                const auto instr_name   = with_carry ? "SBC" : "SUB";
                const auto operand_name = this->opcode & 0x40 ? "d8" : "(HL)";

                log(fmt::format("{} A, {}\n", instr_name, operand_name));
                this->cycle++;
                this->pc++;
            } else if (this->cycle == 1) {
                const bool with_carry      = this->opcode & 0x08;
                const auto instr_name      = with_carry ? "SBC" : "SUB";
                const auto operand_address = this->opcode & 0x40 ? this->pc++ : this->hl.r16;
                const auto operand         = bus.read(operand_address);
                this->a()                  = this->get_sub(operand, with_carry);

                const auto operand_str = this->opcode & 0x40
                                             ? fmt::format("${:02X}", operand)
                                             : fmt::format("${:02X} (@ ${:04X})", operand, operand_address);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A = A {} {} = ${:02X}\n", instr_name, operand_str, this->a()));
                this->cycle = 0;
            }
            break;

        case 0x90: // SUB
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x97:

        case 0x98: // SBC
        case 0x99:
        case 0x9a:
        case 0x9b:
        case 0x9c:
        case 0x9d:
        case 0x9f:
        {
            const bool with_carry = this->opcode & 0x08;
            const auto instr_name = with_carry ? "SBC" : "SUB";
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);

            log(fmt::format("{} A, {}\n", instr_name, reg_name));

            const auto &reg           = decode_reg8(this->opcode & 0x7);
            this->a() = this->get_sub(reg, with_carry);

            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A {} {} = ${:02X}\n", instr_name, reg_name, this->a()));
            this->pc += 1;
        } break;

        case 0x09: // ADD HL, r16
        case 0x19:
        case 0x29:
        case 0x39:
            if (this->cycle == 0) {
                const auto reg_name = decode_reg16_name((this->opcode>>4) & 0x3);
                log(fmt::format("ADD HL, {}\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {

                auto &reg           = decode_reg16((this->opcode>>4) & 0x3);
                const uint16_t res13  = (this->hl.r16&0x0fff) + (reg & 0x0fff);
                const uint32_t res17 = static_cast<uint32_t>(this->hl.r16) + reg;
                this->hl.r16 = res17 & 0xffff;

                this->set_flag_h(res13 & 0x1000);
                this->set_flag_c(res17 & 0x10000);
                this->set_flag_n(0);
                const auto reg_name = decode_reg16_name((this->opcode>>4) & 0x3);
                log(fmt::format("\t\t\t\t\t\t\t\t\t HL = HL + {} = ${:04X}\n", reg_name, this->hl.r16));
                this->pc += 1;
                this->cycle = 0;
            }
        break;

        case 0xA0: // AND B
        case 0xA1: // AND C
        case 0xA2: // AND D
        case 0xA3: // AND E
        case 0xA4: // AND H
        case 0xA5: // AND L
        case 0xA7: // AND A
        {
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);
            auto &reg           = decode_reg8(this->opcode & 0x7);

            log(fmt::format("AND {}\n", reg_name));
            this->a() &= reg;
            this->flags() = (this->a() == 0) ? 0xA0 : 0x20;
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A AND {} = ${:02X}\n", reg_name, this->a()));
            this->pc += 1;
        } break;

        case 0xA6: // AND A, (HL)
        case 0xE6: // AND A, d8
            if (this->cycle == 0) {
                const auto operand_name = this->opcode & 0x40 ? "d8" : "(HL)";

                log(fmt::format("AND A, {}\n", operand_name));
                this->cycle++;
                this->pc++;
            } else if (this->cycle == 1) {
                const auto operand_address = this->opcode & 0x40 ? this->pc++ : this->hl.r16;
                const auto operand         = bus.read(operand_address);
                this->a() &= operand;
                this->flags() = (this->a() == 0) ? 0xA0 : 0x20;

                const auto operand_str = this->opcode & 0x40
                                             ? fmt::format("${:02X}", operand)
                                             : fmt::format("${:02X} (@ ${:04X})", operand, operand_address);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A = A AND {} = ${:02X}\n", operand_str, this->a()));
                this->cycle = 0;
            }
            break;

        case 0xA8: // XOR B  1010 1000
        case 0xA9: // XOR C  1010 1001
        case 0xAA: // XOR D  1010 1010
        case 0xAB: // XOR E  1010 1011
        case 0xAC: // XOR H  1010 1100
        case 0xAD: // XOR L  1010 1101
        case 0xAF: // XOR A  1010 1111

        case 0xB0: // OR B
        case 0xB1: // OR C
        case 0xB2: // OR D
        case 0xB3: // OR E
        case 0xB4: // OR H
        case 0xB5: // OR L
        case 0xB7: // OR A
        {
            const bool isxor    = (this->opcode & 0b00011000) == 0b00001000;
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);
            auto &reg           = decode_reg8(this->opcode & 0x7);

            const auto instr_name = isxor ? "XOR" : "OR";
            log(fmt::format("{} {}\n", instr_name, reg_name));
            this->a() = isxor ? this->a() ^ reg : this->a() | reg;

            this->flags() = (this->a() == 0) ? 0x80 : 0x00;
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A {} {} = ${:02X}\n", instr_name, reg_name, this->a()));
            this->pc += 1;
        } break;

        case 0xAE: // XOR A, (HL)
        case 0xEE: // XOR A, d8
        case 0xB6: // OR A, (HL)
        case 0xF6: // OR A, d8
            if (this->cycle == 0) {
                const auto instr_name   = this->opcode & 0x08 ? "XOR" : "OR";
                const auto operand_name = this->opcode & 0x40 ? "d8" : "(HL)";

                log(fmt::format("{} A, {}\n", instr_name, operand_name));
                this->cycle++;
                this->pc++;
            } else if (this->cycle == 1) {
                const auto instr_name      = this->opcode & 0x08 ? "XOR" : "OR";
                const auto operand_address = this->opcode & 0x40 ? this->pc++ : this->hl.r16;
                const auto operand         = bus.read(operand_address);
                this->a()                  = this->opcode & 0x08 ? this->a() ^ operand : this->a() | operand;
                this->flags()              = (this->a() == 0) ? 0x80 : 0x00;

                const auto operand_str = this->opcode & 0x40
                                             ? fmt::format("${:02X}", operand)
                                             : fmt::format("${:02X} (@ ${:04X})", operand, operand_address);
                log(fmt::format("\t\t\t\t\t\t\t\t\t A = A {} {} = ${:02X}\n", instr_name, operand_str, this->a()));
                this->cycle = 0;
            }
            break;

        // DEC {B,C,D,E,H,L,A}
        case 0x05:
        case 0x0D:
        case 0x15:
        case 0x1D:
        case 0x25:
        case 0x2D:
        case 0x3D: {
            // single cycle
            const auto reg_name = decode_reg8_name((this->opcode>>3)&0x7);
            auto &reg = decode_reg8((this->opcode>>3)&0x7);

            log(fmt::format("DEC {}\n", reg_name));

            reg = this->get_dec(reg);

            log(fmt::format("\t\t\t\t\t\t\t\t\t {} decremented to ${:02X}\n", reg_name, reg));

            this->pc += 1;
        } break;

        // INC {B,C,D,E,H,L,A}
        case 0x04:
        case 0x0C:
        case 0x14:
        case 0x1C:
        case 0x24:
        case 0x2C:
        case 0x3C: {
            // single cycle
            const auto reg_name = decode_reg8_name((this->opcode >> 3) & 0x7);
            auto &reg           = decode_reg8((this->opcode >> 3) & 0x7);

            log(fmt::format("INC {}\n", reg_name));

            reg = this->get_inc(reg);

            log(fmt::format("\t\t\t\t\t\t\t\t\t {} incremented to ${:02X}\n", reg_name, reg));

            this->pc += 1;
        } break;

        // DEC (HL)
        case 0x35:
            if (this->cycle == 0) {
                log(fmt::format("DEC (HL)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->hl.r16);
                this->cycle++;
            } else if (this->cycle == 2) {
                const auto newval = this->get_dec(this->tmp1);
                bus.write(this->hl.r16, newval);
                log(fmt::format("\t\t\t\t\t\t\t\t\t (HL) (${:04X}) decremented to ${:02X}\n", this->hl.r16, newval));
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        // INC (HL)
        case 0x34:
            if (this->cycle == 0) {
                log(fmt::format("INC (HL)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->hl.r16);
                this->cycle++;
            } else if (this->cycle == 2) {
                const auto newval = this->get_inc(this->tmp1);
                bus.write(this->hl.r16, newval);
                log(fmt::format("\t\t\t\t\t\t\t\t\t (HL) (${:04X}) incremented to ${:02X}\n", this->hl.r16, newval));
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        // DEC {BC,DE,HL,SP}
        case 0x0B:
        case 0x1B:
        case 0x2B:
        case 0x3B:
            if (this->cycle == 0) {
                const auto reg_name = decode_reg16_name((this->opcode >> 4) & 0x3);
                log(fmt::format("DEC {}\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {
                const auto reg_name = decode_reg16_name((this->opcode >> 4) & 0x3);
                auto &reg           = decode_reg16((this->opcode >> 4) & 0x3);
                reg--;
                log(fmt::format("\t\t\t\t\t\t\t\t\t {} decremented ${:04X}\n", reg_name, reg));
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        // INC {BC,DE,HL,SP}
        case 0x03:
        case 0x13:
        case 0x23:
        case 0x33:
            if (this->cycle == 0) {
                const auto reg_name = decode_reg16_name((this->opcode >> 4) & 0x3);
                log(fmt::format("INC {}\n", reg_name));
                this->cycle++;
            } else if (this->cycle == 1) {
                const auto reg_name = decode_reg16_name((this->opcode >> 4) & 0x3);
                auto &reg           = decode_reg16((this->opcode >> 4) & 0x3);
                reg++;
                log(fmt::format("\t\t\t\t\t\t\t\t\t {} incremented ${:04X}\n", reg_name, reg));
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        // Comparison
        case 0xB8:
        case 0xB9:
        case 0xBA:
        case 0xBB:
        case 0xBC:
        case 0xBD:
        case 0xBF: {
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);
            auto &reg           = decode_reg8(this->opcode & 0x7);
            log(fmt::format("CP {}\n", reg_name));
            this->get_sub(reg, false);

            log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) compared to {}\n", this->a(), reg_name));
            this->pc++;
        } break;

        case 0xBE:
        case 0xFE:
            if (this->cycle == 0) {
                const auto operand_name = this->opcode & 0x40 ? "d8" : "(HL)";

                log(fmt::format("CP {}\n", operand_name));
                this->cycle++;
                this->pc++;
            } else if (this->cycle == 1) {
                const auto operand_address = this->opcode & 0x40 ? this->pc++ : this->hl.r16;
                const auto operand         = bus.read(operand_address);

                this->get_sub(operand, false);

                const auto operand_str = this->opcode & 0x40
                                             ? fmt::format("${:02X}", operand)
                                             : fmt::format("${:02X} (@ ${:04X})", operand, operand_address);

                log(fmt::format("\t\t\t\t\t\t\t\t\t A (${:02X}) compared to {}\n", this->a(), operand_str));
                this->cycle = 0;
            }
            break;

        //-------------------------------------------------------
        // Jumps
        //-------------------------------------------------------

        // relative jumps
        case 0x18:
            if(this->cycle == 0) {
                log(fmt::format("JR s8\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc+1);
                this->cycle++;
            } else if (this->cycle == 2) {
                const int16_t offset = static_cast<int8_t>(this->tmp1);
                this->pc += offset+2;
                log(fmt::format("\t\t\t\t\t\t\t\t\t doing relative jump with offset {} to ${:04X}\n", offset, this->pc));
                this->cycle=0;
            }
            break;

        case 0x20: // JR COND, s8
        case 0x28:
        case 0x30:
        case 0x38:
            if (this->cycle == 0) {
                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const auto cond_str  = cond_bits == 0 ? "NZ" : cond_bits == 1 ? "Z" : cond_bits == 2 ? "NC" : "C";
                log(fmt::format("JR {}, s8\n", cond_str));
                this->cycle++;
            } else if (this->cycle == 1) {

                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const bool cond      = cond_bits == 0   ? !this->get_flag_z()
                                       : cond_bits == 1 ? this->get_flag_z()
                                       : cond_bits == 2 ? !this->get_flag_c()
                                                        : this->get_flag_c();
                if (cond) {
                    this->tmp1 = bus.read(this->pc + 1);
                    this->cycle++;
                } else {
                    log(fmt::format("\t\t\t\t\t\t\t\t\t relative jump NOT taken\n"));
                    this->pc += 2;
                    this->cycle = 0;
                }
            } else if (this->cycle == 2) {
                const int16_t offset = static_cast<int8_t>(this->tmp1);
                this->pc += offset + 2; // extra 2 ?
                log(fmt::format("\t\t\t\t\t\t\t\t\t relative jump taken with offset {} to ${:04X}\n", offset, this->pc));
                this->cycle = 0;
            }
            break;

        case 0xE9: {
                log(fmt::format("JP HL\n"));
                this->pc = this->hl.r16;
                log(fmt::format("\t\t\t\t\t\t\t\t\t jumping to address in HL (${:04X})\n", this->pc));
            }
            break;

        case 0xC3:
            if(this->cycle == 0) {
                log(fmt::format("JP a16\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1); // addr_l
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2); // addr_h
                this->cycle++;
            } else if (this->cycle == 3) {
                const uint16_t new_pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t Jumping to: ${:04X}\n", new_pc));
                this->pc = new_pc;
                this->cycle = 0;
            }
            break;

        case 0xC2: // JP COND, a16
        case 0xCA:
        case 0xD2:
        case 0xDA:
            if (this->cycle == 0) {
                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const auto cond_str  = cond_bits == 0 ? "NZ" : cond_bits == 1 ? "Z" : cond_bits == 2 ? "NC" : "C";
                log(fmt::format("JP {}, a16\n", cond_str));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2);
                this->pc += 3;

                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const bool cond      = cond_bits == 0   ? !this->get_flag_z()
                                       : cond_bits == 1 ? this->get_flag_z()
                                       : cond_bits == 2 ? !this->get_flag_c()
                                                        : this->get_flag_c();
                if (cond) {
                    this->cycle++;
                } else {
                    log(fmt::format("\t\t\t\t\t\t\t\t\t absolute jump NOT taken\n"));
                    this->cycle = 0;
                }
            } else if (this->cycle == 3) {
                const uint16_t new_pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t conditional jump taken to ${:04X}\n", new_pc));
                this->pc = new_pc;
                this->cycle = 0;
            }
            break;

        // Call
        case 0xCD:
            if(this->cycle == 0) {
                log(fmt::format("CALL a16\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1); // addr_l
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2); // addr_h
                this->cycle++;
            } else if (this->cycle == 3) {
                this->pc += 3;
                this->sp--;
                bus.write(this->sp, (this->pc >> 8)&0xff);
                this->cycle++;
            } else if (this->cycle == 4) {
                this->sp--;
                bus.write(this->sp, this->pc&0xff);
                this->cycle++;
            } else if (this->cycle == 5) {
                this->pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t Calling subroutine at: ${:04X}\n", this->pc));
                this->cycle = 0;
            }
            break;

        case 0xC4:
        case 0xCC:
        case 0xD4: // CALL <COND>, a16
        case 0xDC:
            if (this->cycle == 0) {
                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const auto cond_str  = cond_bits == 0 ? "NZ" : cond_bits == 1 ? "Z" : cond_bits == 2 ? "NC" : "C";
                log(fmt::format("CALL {}, a16\n", cond_str));
                this->cycle++;
            } else if (this->cycle == 1) {
                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const bool cond      = cond_bits == 0   ? !this->get_flag_z()
                                       : cond_bits == 1 ? this->get_flag_z()
                                       : cond_bits == 2 ? !this->get_flag_c()
                                                        : this->get_flag_c();
                if (cond) {
                    this->tmp1 = bus.read(this->pc + 1); // addr_l
                    this->cycle++;
                } else {
                    log(fmt::format("\t\t\t\t\t\t\t\t\t conditional call NOT taken\n"));
                    this->pc += 3;
                    this->cycle = 0;
                }
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2); // addr_h
                this->cycle++;
            } else if (this->cycle == 3) {
                this->pc += 3;
                this->sp--;
                bus.write(this->sp, (this->pc >> 8) & 0xff);
                this->cycle++;
            } else if (this->cycle == 4) {
                this->sp--;
                bus.write(this->sp, this->pc & 0xff);
                this->cycle++;
            } else if (this->cycle == 5) {
                this->pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t Conditional calling to subroutine at: ${:04X}\n", this->pc));
                this->cycle = 0;
            }
            break;

        // RST n
        case 0xC7:
        case 0xD7:
        case 0xE7:
        case 0xF7:
        case 0xCF:
        case 0xDF:
        case 0xEF:
        case 0xFF:
            if(this->cycle == 0) {
                this->tmp1 = (this->opcode>>3)&0x7;
                log(fmt::format("RST {}\n",this->tmp1));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->pc += 1;
                this->sp--;
                bus.write(this->sp, (this->pc >> 8)&0xff);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->sp--;
                bus.write(this->sp, this->pc&0xff);
                this->cycle++;
            } else if (this->cycle == 3) {
                this->pc = 8*this->tmp1;
                log(fmt::format("\t\t\t\t\t\t\t\t\t Calling subroutine at: ${:04X}\n", this->pc));
                this->cycle = 0;
            }
            break;



        // Return
        case 0xC9:
            if(this->cycle == 0) {
                log(fmt::format("RET\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->sp);
                this->sp++;
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->sp);
                this->sp++;
                this->cycle++;
            } else if (this->cycle == 3) {
                this->pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t Returning from subroutine\n"));
                this->cycle = 0;
            }
            break;

        case 0xC0:
        case 0xC8:
        case 0xD0: // RET <COND>
        case 0xD8:
            if (this->cycle == 0) {
                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const auto cond_str  = cond_bits == 0 ? "NZ" : cond_bits == 1 ? "Z" : cond_bits == 2 ? "NC" : "C";
                log(fmt::format("RET {}\n", cond_str));
                this->cycle++;
            } else if (this->cycle == 1) {
                const auto cond_bits = (this->opcode >> 3) & 0x3;
                const bool cond      = cond_bits == 0   ? !this->get_flag_z()
                                       : cond_bits == 1 ? this->get_flag_z()
                                       : cond_bits == 2 ? !this->get_flag_c()
                                                        : this->get_flag_c();
                if (cond) {
                    this->cycle++;
                } else {
                    log(fmt::format("\t\t\t\t\t\t\t\t\t conditional return NOT taken\n"));
                    this->pc += 1;
                    this->cycle = 0;
                }
            } else if (this->cycle == 2) {
                this->tmp1 = bus.read(this->sp); // LSB
                this->sp++;
                this->cycle++;
            } else if (this->cycle == 3) {
                this->tmp2 = bus.read(this->sp); // MSB
                this->sp++;
                this->cycle++;
            } else if (this->cycle == 4) {
                this->pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t conditional return taken\n"));
                this->cycle = 0;
            }
            break;

        case 0xD9:
            if(this->cycle == 0) {
                log(fmt::format("RETI\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->sp); // LSB
                this->sp++;
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->sp); // MSB
                this->sp++;
                this->cycle++;
            } else if (this->cycle == 3) {
                this->ime = true;
                this->pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                log(fmt::format("\t\t\t\t\t\t\t\t\t Returning from interrupt handler (interrupts reenabled)\n"));
                this->cycle = 0;
            }
            break;


        //-------------------------------------------------------
        // Interrupt stuff
        //-------------------------------------------------------
        case 0xF3:
            // single cycle
            log(fmt::format("DI\n"));
            this->ime = false;
            log(fmt::format("\t\t\t\t\t\t\t\t\t Maskable interrupts disabled\n"));
            this->pc++;
            break;
        case 0xFB:
            // TODO: EI should only apply after one extra cycle
            // single cycle
            log(fmt::format("EI\n"));
            this->ime = true;
            log(fmt::format("\t\t\t\t\t\t\t\t\t Maskable interrupts enabled\n"));
            this->pc++;
            break;

        //-------------------------------------------------------
        // Extended instruction set
        //-------------------------------------------------------

        case 0xCB:
            if (this->cycle == 0) {
                log(fmt::format("16-bit instruction\n"));
                this->cycle++;
                return;
            }

            if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                log(fmt::format("\t\t\t\t\t\t\t\t read extended opcode: ${:02X} -> ", this->tmp1));
            }

            if ((this->tmp1 & 0b11111000) == 0b00000000) { // RLC
                if ((this->tmp1 & 0x7) == 0x06) {          // RLC (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("RLC {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->rlc(reg, true);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t 8-bit rotate left of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00001000) { // RRC
                if ((this->tmp1 & 0x7) == 0x06) {                 // RRC (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("RRC {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->rrc(reg, true);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t 8-bit rotate right of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00010000) { // RL
                if ((this->tmp1 & 0x7) == 0x06) {                 // RL (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("RL {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->rl(reg, true);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t 9-bit rotate left of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00011000) { // RR
                if ((this->tmp1 & 0x7) == 0x06) {                 // RR (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("RR {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->rr(reg, true);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t 9-bit rotate right of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00100000) { // SLA
                if ((this->tmp1 & 0x7) == 0x06) {                 // SLA (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("SLA {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->set_flag_c(reg & 0x80);
                        reg = reg << 1;
                        this->set_flag_z(reg == 0);
                        this->set_flag_h(false);
                        this->set_flag_n(false);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Left shift of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00101000) { // SRA
                if ((this->tmp1 & 0x7) == 0x06) {                 // SRA (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("SRA {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->set_flag_c(reg & 0x01);
                        reg = (reg & 0x80) | (reg >> 1); // keep msb
                        this->set_flag_z(reg == 0);
                        this->set_flag_h(false);
                        this->set_flag_n(false);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Arithmetic right shift of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00110000) { // SWAP
                if ((this->tmp1 & 0x7) == 0x06) {                 // SWAP (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("SWAP {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        reg       = ((reg & 0xf) << 4) | ((reg >> 4) & 0xf);
                        this->set_flag_z(reg == 0);
                        this->set_flag_h(false);
                        this->set_flag_c(false);
                        this->set_flag_n(false);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Swapping high and low nibbles of {} = ${:02X}\n",
                                        reg_name,
                                        reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11111000) == 0b00111000) { // SRL
                if ((this->tmp1 & 0x7) == 0x06) {                 // SRL (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        log(fmt::format("SRL {}\n", reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->set_flag_c(reg & 0x01);
                        reg = reg >> 1; // unsigned shift drop msb correctly
                        this->set_flag_z(reg == 0);
                        this->set_flag_h(false);
                        this->set_flag_n(false);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Logical right shift of {} = ${:02X}\n", reg_name, reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11000000) == 0b10000000) { // RES <bit>, r
                if ((this->tmp1 & 0x7) == 0x06) {                 // RES <bit>, (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        const auto bit      = (this->tmp1 >> 3) & 0x7;

                        log(fmt::format("RES {}, {}\n", bit, reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        reg &= ~(1 << bit);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Resetting bit {} of register {} = ${:02X}\n",
                                        bit,
                                        reg_name,
                                        reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11000000) == 0b11000000) { // SET <bit>, r
                if ((this->tmp1 & 0x7) == 0x6) {                  // SET <bit>, (HL)
                    log(fmt::format("???\n"));
                    throw std::runtime_error(
                        fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        const auto bit      = (this->tmp1 >> 3) & 0x7;

                        log(fmt::format("SET {}, {}\n", bit, reg_name));
                        auto &reg = decode_reg8(this->tmp1 & 0x7);
                        reg |= (1 << bit);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Setting bit {} of register {} = ${:02X}\n",
                                        bit,
                                        reg_name,
                                        reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else if ((this->tmp1 & 0b11000000) == 0b01000000) { // BIT <bit>, r
                if ((this->tmp1 & 0x7) == 0x6) {                  // BIT <bit>, (HL)
                    if (this->cycle == 1) {
                        const auto bit = (this->tmp1 >> 3) & 0x7;
                        log(fmt::format("BIT {}, (HL)\n", bit));
                        this->cycle++;
                    } else if (this->cycle == 2) {
                        const auto bit = (this->tmp1 >> 3) & 0x7;
                        const auto &op = bus.read(this->hl.r16);
                        this->set_flag_z(op & (1 << bit));
                        this->set_flag_n(false);
                        this->set_flag_h(true);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Testing bit {} of memory location (HL)=${:04X} = ${:02X}\n",
                                        bit,
                                        this->hl.r16,
                                        op));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                } else {
                    if (this->cycle == 1) {
                        const auto reg_name = decode_reg8_name(this->tmp1 & 0x7);
                        const auto bit      = (this->tmp1 >> 3) & 0x7;

                        log(fmt::format("BIT {}, {}\n", bit, reg_name));
                        const auto &reg = decode_reg8(this->tmp1 & 0x7);
                        this->set_flag_z(reg & (1 << bit));
                        this->set_flag_n(false);
                        this->set_flag_h(true);

                        log(fmt::format("\t\t\t\t\t\t\t\t\t Testing bit {} of register {} = ${:02X}\n",
                                        bit,
                                        reg_name,
                                        reg));
                        this->pc += 2;
                        this->cycle = 0;
                    }
                }
            } else {
                log(fmt::format("???\n"));
                throw std::runtime_error(fmt::format("UNKNOWN EXTENDED OPCODE ${:02X} at PC=${:04X}", this->tmp1, this->pc));
            }

            break;

        default:
            log(fmt::format("???\n"));
            throw std::runtime_error(fmt::format("UNKNOWN OPCODE ${:02X} at PC=${:04X}", this->opcode, this->pc));
    }
}

void Cpu::dump(std::ostream &os) const {

    os << fmt::format("  af: {:04X}\n", this->af.r16);
    os << fmt::format("  bc: {:04X}\n", this->bc.r16);
    os << fmt::format("  de: {:04X}\n", this->de.r16);
    os << fmt::format("  hl: {:04X}\n", this->hl.r16);
    os << fmt::format("  pc: {:04X}\n", this->pc);
    os << fmt::format("  sp: {:04X}\n", this->sp);
    os << fmt::format(" ime: {}\n", this->ime);
}
