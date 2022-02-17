#include "cpu.h"

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

    this->set_flag_h(res5 & 0x10 != 0);
    this->set_flag_c(res9&0x100 != 0);
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

void Cpu::do_tick(Bus &bus) {

    if(this->cycle == 0) {
        this->opcode = bus.read(this->pc);
        log(fmt::format("\t\t\t\t\t\t\t\t read opcode: ${:02X} -> ", this->opcode));
    }

    switch (this->opcode) {
        case 0x00:
            log(fmt::format("NOP\n"));
            this->pc += 1;
            this->cycle = 0; // start over
            break;

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
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (${:04X})\n", this->a(), addr));
                bus.write(addr, this->a());
                this->pc += 2;
                this->cycle = 0;
            }
            break;
        case 0xF0:
            if(this->cycle == 0) {
                log(fmt::format("LD A, (a8)\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                const uint16_t addr = 0xff00 | this->tmp1;
                this->a() = bus.read(addr);
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) read from (${:04X})\n", this->a(), addr));
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
                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (${:04X})\n", this->a(), addr));
                bus.write(addr, this->a());
                this->pc += 1;
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
                this->tmp1 = bus.read(this->sp);
                this->sp++;
                this->cycle++;
            } else {
                this->tmp2 = bus.read(this->sp);
                this->sp++;
                reg = static_cast<uint16_t>(this->tmp1) << 8 | this->tmp2;
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
                log(fmt::format("\t\t\t\t\t\t\t\t\t pushed {} from to stack: ${:04X}\n", reg_name, reg));
                this->pc += 1;
                this->cycle = 0;
            }
        } break;

            //-------------------------------------------------------
            // ALU
            //-------------------------------------------------------

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
            this->a()     &= reg;
            this->flags() = (this->a() == 0) ? 0x80 : 0x00;
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A AND {} = ${:02X}\n", reg_name, this->a()));
            this->pc += 1;
        } break;

        case 0xE6: // AND A, d8
            if (this->cycle == 0) {
                log(fmt::format("LD A, d8\n"));
                this->cycle++;
            } else if (this->cycle == 1) {
                const auto v = bus.read(this->pc + 1);
                this->a() &= v;
                this->flags() = (this->a() == 0) ? 0b10100000 : 0b00100000;
                log(fmt::format("\t\t\t\t\t\t\t\t\t A = A AND ${:02X} = ${:02X}\n", v, this->a()));
                this->pc += 2;
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
        {
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);
            auto &reg           = decode_reg8(this->opcode & 0x7);

            log(fmt::format("XOR {}\n", reg_name));
            this->a()     ^= reg;
            this->flags() = (this->a() == 0) ? 0x80 : 0x00;
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A XOR {} = ${:02X}\n", reg_name, this->a()));
            this->pc += 1;
        } break;

        case 0xB0: // OR B
        case 0xB1: // OR C
        case 0xB2: // OR D
        case 0xB3: // OR E
        case 0xB4: // OR H
        case 0xB5: // OR L
        case 0xB7: // OR A
        {
            const auto reg_name = decode_reg8_name(this->opcode & 0x7);
            auto &reg           = decode_reg8(this->opcode & 0x7);

            log(fmt::format("OR {}\n", reg_name));
            this->a()     |= reg;
            this->flags() = (this->a() == 0) ? 0x80 : 0x00;
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = A OR {} = ${:02X}\n", reg_name, this->a()));
            this->pc += 1;
        } break;

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

            reg -= 1;
            this->set_flag_z(reg==0);
            this->set_flag_n(true);

            log(fmt::format("\t\t\t\t\t\t\t\t\t {} decremented to ${:02X}\n", reg_name, reg));

            this->pc += 1;
            break;
        }

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

            reg += 1;
            this->set_flag_z(reg == 0);
            this->set_flag_n(false);

            log(fmt::format("\t\t\t\t\t\t\t\t\t {} incremented to ${:02X}\n", reg_name, reg));

            this->pc += 1;
        } break;

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
        case 0xFE:
            if(this->cycle == 0) {
                log(fmt::format("CP d8\n"));
                this->cycle++;
            } else {
                const auto val = bus.read(this->pc + 1);
                this->get_sub(val, false);

                log(fmt::format("\t\t\t\t\t\t\t\t\t a (${:02X}) compared to ${:02X}\n", this->a(), val));

                this->pc += 2;
                this->cycle=0;
            }
            break;

        // Complement
        case 0x2F: // CPL
        {
            log(fmt::format("CPL\n"));
            this->a()     = ~(this->a());
            this->flags() = (this->a() & 0b10010000) | 0b01100000;
            log(fmt::format("\t\t\t\t\t\t\t\t\t A = ~A = ${:02X}\n", this->a()));
            this->pc += 1;
        } break;

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
                log(fmt::format("JP nn nn\n"));
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
                bus.write(this->sp, this->pc&0xff);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->sp--;
                bus.write(this->sp, (this->pc >> 8)&0xff);
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
                log(fmt::format("\t\t\t\t\t\t\t\t\t Returning from subroutine\n", this->pc));
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

            switch (this->tmp1) {
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x37:
                if (this->cycle == 1) {
                    const auto reg_name = decode_reg8_name((this->opcode >> 3) & 0x7);
                    log(fmt::format("SWAP {}\n", reg_name));
                    auto &reg = decode_reg8((this->opcode >> 3) & 0x7);
                    reg       = ((reg & 0xf) << 4) | ((reg >> 4) & 0xf);

                    log(fmt::format("\t\t\t\t\t\t\t\t\t Swapping high and low nibbles of {} = ${:02X}\n", reg_name, reg));
                    this->pc += 2;
                    this->cycle = 0;
                }
                break;
            default:
                log(fmt::format("???\n"));
                throw std::runtime_error(fmt::format("UNKNOWN EXTENDED OPCODE: ${:02X}", this->tmp1));
            }

            break;

        default:
            log(fmt::format("???\n"));
            throw std::runtime_error(fmt::format("UNKNOWN OPCODE: ${:02X}", this->opcode));
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
