#include "cpu.h"

#include "bus.h"

#include <fmt/core.h>

void Cpu::reset() {
    this->a     = 0x01; // on GB/SGB. 0xff on GBP, 0x11 on GBC
    this->flags = 0xb0;
    this->bc.r16 = 0x0013; // on GB/SGB/GBP/GBC
    this->de.r16 = 0x00d8;
    this->hl.r16 = 0x014d;
    this->sp    = 0xfffe;
    this->pc    = 0x0100;
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
    default: // 6
        return this->a;
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
    default: // 6
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

void Cpu::do_tick(Bus &bus) {

    if(this->cycle == 0) {
        this->opcode = bus.read(this->pc);
        fmt::print("\t\t\t\t\t\t\t\t read opcode: ${:02X} -> ", opcode);
    }

    switch (this->opcode) {
        case 0x00:
            fmt::print("NOP\n");
            this->pc += 1;
            this->cycle = 0; // start over
            break;

        //-------------------------------------------------------
        // LD
        //-------------------------------------------------------
        case 0x06: // LD B, d8    0000 0110
        case 0x0E: // LD C, d8    0000 1110
        case 0x16: // LD D, d8    0001 0110
        case 0x1E: // LD E, d8    0001 1110
        case 0x26: // LD H, d8    0010 0110
        case 0x2E: // LD L, d8    0010 1110
        case 0x3E: // LD A, d8    0011 1110
        {
            if (this->cycle == 0) {
                const auto reg_name = decode_reg8_name((this->opcode>>3)&0x7);
                fmt::print("LD {}, d8\n", reg_name);
                this->cycle++;
            } else {
                const auto reg_name = decode_reg8_name((this->opcode>>3)&0x7);
                auto &reg       = decode_reg8((this->opcode>>3)&0x7);
                reg             = bus.read(this->pc + 1);
                fmt::print("\t\t\t\t\t\t\t\t\t {} set to ${:02X}\n", reg_name, reg);
                this->pc += 2;
                this->cycle = 0;
            }
        } break;
        case 0x36: // LD (HL), d8
        {
            if (this->cycle == 0) {
                fmt::print("LD (HL), d8\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                bus.write(this->hl.r16, this->tmp1);
                fmt::print("\t\t\t\t\t\t\t\t\t ${:02X} stored to (hl) (${:04X})\n",
                           this->tmp1,
                           this->hl.r16);

                this->pc += 2;
                this->cycle = 0;
            }
        } break;
        case 0x32:
            if (this->cycle == 0) {
                fmt::print("LD (HL-), A\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                bus.write(this->hl.r16, this->a);
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (hl) (${:04X}), and hl decremented\n",
                           this->a,
                           this->hl.r16);
                this->hl.r16--;
                this->pc += 1;
                this->cycle = 0;
            }
            break;
        case 0x2A: // LD A, (HL+)
            if (this->cycle == 0) {
                fmt::print("LD A, (HL+)\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->a = bus.read(this->hl.r16);
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) read from (hl) (${:04X}), and hl incremented\n",
                           this->a,
                           this->hl.r16);
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
                fmt::print("LD {}, d16\n", reg_name);
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                const auto reg_name = decode_reg16_name((this->opcode>>4)&0x3);
                auto &reg           = decode_reg16((this->opcode>>4)&0x3);

                reg = static_cast<uint16_t>(bus.read(this->pc + 2))<<8 | this->tmp1;

                fmt::print("\t\t\t\t\t\t\t\t\t {} set to ${:04X}\n", reg_name, reg);
                this->pc += 3;
                this->cycle = 0;
            }
            break;

        // store A to immediate address
        case 0xEA:
            if (this->cycle == 0) {
                fmt::print("LD (a16), A\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2);
                this->cycle++;
            } else if (this->cycle == 3) {
                const uint16_t addr = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                bus.write(addr, this->a);
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to ${:04X}\n",
                           this->a,
                           addr);
                this->pc += 3;
                this->cycle = 0;
            }
            break;

        // IO/HRAM PAGE LD instructions
        case 0xE0:
            if (this->cycle == 0) {
                fmt::print("LD (a8), A\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                const uint16_t addr = 0xff00 | this->tmp1;
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (${:04X})\n", this->a, addr);
                bus.write(addr, this->a);
                this->pc += 2;
                this->cycle = 0;
            }
            break;
        case 0xF0:
            if(this->cycle == 0) {
                fmt::print("LD A, (a8)\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1);
                this->cycle++;
            } else {
                const uint16_t addr = 0xff00 | this->tmp1;
                this->a = bus.read(addr);
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) read from (${:04X})\n", this->a, addr);
                this->pc += 2;
                this->cycle = 0;
            }
            break;
        case 0xE2:
            if (this->cycle == 0) {
                fmt::print("LD (C), A\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                const uint16_t addr = 0xff00 | this->bc.r8.lo; // (0xff00|c)
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (${:04X})\n", this->a, addr);
                bus.write(addr, this->a);
                this->pc += 1;
                this->cycle = 0;
            }
            break;

        //-------------------------------------------------------
        // ALU
        //-------------------------------------------------------
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

            fmt::print("XOR {}\n", reg_name);
            this->a     = this->a ^ reg;
            this->flags = (this->a == 0) ? 0x80 : 0x00;
            fmt::print("\t\t\t\t\t\t\t\t\t A = A XOR {} = ${:02X}\n", reg_name, this->a);
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

            fmt::print("DEC {}\n", reg_name);

            reg -= 1;
            this->set_flag_z(reg==0);
            this->set_flag_n(true);

            fmt::print("\t\t\t\t\t\t\t\t\t {} decremented to ${:02X}\n", reg_name, reg);

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
            const auto reg_name = decode_reg8_name((this->opcode>>3)&0x7);
            auto &reg = decode_reg8((this->opcode>>3)&0x7);

            fmt::print("INC {}\n", reg_name);

            reg += 1;
            this->set_flag_z(reg==0);
            this->set_flag_n(false);

            fmt::print("\t\t\t\t\t\t\t\t\t {} incremented to ${:02X}\n", reg_name, reg);

            this->pc += 1;
            break;
        }

        // Comparison
        case 0xFE:
            if(this->cycle == 0) {
                fmt::print("CP d8\n");
                this->cycle++;
            } else {
                const auto val = bus.read(this->pc + 1);
                this->set_flag_z(this->a == val);
                this->set_flag_n(true);

                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) compared to ${:02X}\n", this->a, val);

                this->pc += 2;
                this->cycle=0;
            }
            break;

        // jumps
        case 0x20:
            if(this->cycle == 0) {
                fmt::print("JR NZ, s8\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                if(!get_flag_z()) {
                    this->tmp1 = bus.read(this->pc+1);
                    this->cycle++;
                } else {
                    fmt::print("\t\t\t\t\t\t\t\t\t relative jump NOT taken\n");
                    this->pc += 2;
                    this->cycle=0;
                }
            } else if (this->cycle == 2) {
                const int16_t offset = static_cast<int8_t>(this->tmp1);
                this->pc += offset+2; // extra 2 ?
                fmt::print("\t\t\t\t\t\t\t\t\t relative jump taken with offset {} to ${:04X}\n", offset, this->pc);
                this->cycle=0;
            }
            break;
        case 0xC3:
            if(this->cycle == 0) {
                fmt::print("JP nn nn\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1); // addr_l
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2); // addr_h
                this->cycle++;
            } else if (this->cycle == 3) {
                const uint16_t new_pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                fmt::print("\t\t\t\t\t\t\t\t\t Jumping to: ${:04X}\n", new_pc);
                this->pc = new_pc;
                this->cycle = 0;
            }
            break;
        case 0xCD:
            if(this->cycle == 0) {
                fmt::print("CALL a16\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->tmp1 = bus.read(this->pc + 1); // addr_l
                this->cycle++;
            } else if (this->cycle == 2) {
                this->tmp2 = bus.read(this->pc + 2); // addr_h
                this->cycle++;
            } else if (this->cycle == 3) {
                this->pc += 2;
                this->sp--;
                bus.write(this->sp, (this->pc >> 8)&0xff);
                this->cycle++;
            } else if (this->cycle == 4) {
                this->sp--;
                bus.write(this->sp, this->pc&0xff);
                this->cycle++;
            } else if (this->cycle == 5) {
                this->pc = (static_cast<uint16_t>(this->tmp2) << 8) | static_cast<uint16_t>(this->tmp1);
                fmt::print("\t\t\t\t\t\t\t\t\t Calling subroutine at: ${:04X}\n", this->pc);
                this->cycle = 0;
            }
            break;

        // interrupt stuff
        case 0xF3:
            // single cycle
            fmt::print("DI\n");
            this->ime = false;
            fmt::print("\t\t\t\t\t\t\t\t\t All interrupts disabled\n");
            this->pc++;
            break;

        default:

            fmt::print("???\n");
            throw std::runtime_error(fmt::format("UNKNOWN OPCODE: ${:02X}", this->opcode));
    }
}

void Cpu::dump(std::ostream &os) const {

    os << fmt::format("  a: {:02X} f: {:02X}\n", this->a, this->flags);
    os << fmt::format("  bc: {:04X}\n", this->bc.r16);
    os << fmt::format("  de: {:04X}\n", this->de.r16);
    os << fmt::format("  hl: {:04X}\n", this->hl.r16);
    os << fmt::format("  pc: {:04X}\n", this->pc);
    os << fmt::format("  sp: {:04X}\n", this->sp);
}
