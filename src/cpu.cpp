#include "cpu.h"

#include "bus.h"

#include <fmt/core.h>

void Cpu::reset() {
    this->a     = 0x01; // on GB/SGB. 0xff on GBP, 0x11 on GBC
    this->flags = 0xb0;
    this->b     = 0x00; // on GB/SGB/GBP/GBC
    this->c     = 0x13;
    this->d     = 0x00;
    this->e     = 0xd8;
    this->h     = 0x01;
    this->l     = 0x4d;
    this->sp    = 0xfffe;
    this->pc    = 0x100;
}

uint8_t &Cpu::decode_reg(uint8_t bits) {
    switch(bits) {
    case 0:
        return this->b;
    case 1:
        return this->c;
    case 2:
        return this->d;
    case 3:
        return this->e;
    case 4:
        return this->h;
    case 5:
        return this->l;
    default: // 6
        return this->a;
    }
}

std::string Cpu::decode_reg_name(uint8_t bits) const {
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

void Cpu::do_tick(Bus &bus) {

    if(this->cycle == 0) {
        this->opcode = bus.read(this->pc);
        fmt::print("\t\t\t\t\t\t\t\t\t read opcode: ${:02X} -> ", opcode);
    }

    switch (this->opcode) {
        case 0x00:
            fmt::print("NOP\n");
            this->pc += 1;
            this->cycle = 0; // start over
            break;

        // LD
        case 0x06: // LD B, d8    0000 0110
        case 0x0E: // LD C, d8    0000 1110
        case 0x16: // LD D, d8    0001 0110
        case 0x1E: // LD E, d8    0001 1110
        case 0x26: // LD H, d8    0010 0110
        case 0x2E: // LD L, d8    0010 1110
        case 0x3E: // LD A, d8    0011 1110
        {
            if (this->cycle == 0) {
                const auto reg_name = decode_reg_name((this->opcode>>3)&0x7);
                fmt::print("LD {}, d8\n", reg_name);
                this->cycle++;
            } else {
                const auto reg_name = decode_reg_name((this->opcode>>3)&0x7);
                auto &reg       = decode_reg((this->opcode>>3)&0x7);
                reg             = bus.read(this->pc + 1);
                fmt::print("\t\t\t\t\t\t\t\t\t {} set to ${:02X}\n", reg_name, reg);
                this->pc += 2;
                this->cycle = 0;
            }
        } break;
        case 0x21:
            if (this->cycle == 0) {
                fmt::print("LD HL, d16\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                this->l = bus.read(this->pc + 1);
                this->cycle++;
            } else if (this->cycle == 2) {
                this->h = bus.read(this->pc + 2);
                fmt::print("\t\t\t\t\t\t\t\t\t hl set to ${:02X}{:02X}\n", this->h, this->l);
                this->pc += 3;
                this->cycle = 0;
            }
            break;
        case 0x32:
            if (this->cycle == 0) {
                fmt::print("LD (HL-), A\n");
                this->cycle++;
            } else if (this->cycle == 1) {
                uint16_t hl = this->l | (static_cast<uint16_t>(this->h) << 8);
                bus.write(hl, this->a);
                fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (hl) (${:04X}), and hl decremented\n",
                           this->a,
                           hl);
                hl--;
                this->l = hl & 0xff;
                this->h = (hl >> 8) & 0xff;
                this->pc += 1;
                this->cycle = 0;
            }
            break;
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
            const auto reg_name = decode_reg_name(this->opcode & 0x7);
            auto &reg           = decode_reg(this->opcode & 0x7);

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
            const auto reg_name = decode_reg_name((this->opcode>>3)&0x7);
            auto &reg = decode_reg((this->opcode>>3)&0x7);

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
            const auto reg_name = decode_reg_name((this->opcode>>3)&0x7);
            auto &reg = decode_reg((this->opcode>>3)&0x7);

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

        // interrupt stuff
        case 0xF3:
            // single cycle
            fmt::print("DI\n");
            this->ime = false;
            fmt::print("\t\t\t\t\t\t\t\t\t All interrupts disabled\n");
            this->pc++;
            break;

        default:
            fmt::print("UNKOWN\n");
            throw std::runtime_error("CPU INSTR");
    }
}
