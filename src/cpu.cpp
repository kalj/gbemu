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

void Cpu::do_instruction(Bus &bus) {

    auto opcode = bus.read(this->pc);
    fmt::print("Read opcode: ${:02X} -> ", opcode);
    switch (opcode) {
        case 0x00:
            fmt::print("NOP\n");
            this->pc += 1;
            break;

        // LD
        case 0x3E:
            fmt::print("LD A, d8\n");
            this->a = bus.read(this->pc + 1);
            fmt::print("\t\t\t\t\t\t\t\t\t a set to ${:02X}\n", this->a);
            this->pc += 2;
            break;
        case 0x06:
            fmt::print("LD B, d8\n");
            this->b = bus.read(this->pc + 1);
            fmt::print("\t\t\t\t\t\t\t\t\t b set to ${:02X}\n", this->b);
            this->pc += 2;
            break;
        case 0x0E:
            fmt::print("LD C, d8\n");
            this->c = bus.read(this->pc + 1);
            fmt::print("\t\t\t\t\t\t\t\t\t c set to ${:02X}\n", this->c);
            this->pc += 2;
            break;
        case 0x21:
            fmt::print("LD HL, d16\n");
            this->l = bus.read(this->pc + 1);
            this->h = bus.read(this->pc + 2);
            fmt::print("\t\t\t\t\t\t\t\t\t hl set to ${:02X}{:02X}\n", this->h, this->l);
            this->pc += 3;
            break;
        case 0x32: {
            fmt::print("LD (HL-), A\n");
            uint16_t hl = this->l | (static_cast<uint16_t>(this->h) << 8);
            bus.write(hl, this->a);
            fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (hl) (${:04X}), and hl decremented\n", this->a, hl);
            hl--;
            this->l = hl & 0xff;
            this->h = (hl >> 8) & 0xff;
            this->pc += 1;
            break;
        }
        case 0xE0: {
            fmt::print("LD (a8), A\n");
            const uint16_t addr = 0xff00 | bus.read(this->pc+1);
            fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) stored to (${:04X})\n", this->a, addr);
            bus.write(addr, this->a);
            this->pc += 2;
            break;
        }
        case 0xF0: {
            fmt::print("LD A, (a8)\n");
            const uint16_t addr = 0xff00 | bus.read(this->pc+1);
            this->a = bus.read(addr);
            fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) read from (${:04X})\n", this->a, addr);
            this->pc += 2;
            break;
        }

        //-------------------------------------------------------
        // ALU
        //-------------------------------------------------------
        case 0xAF:
            fmt::print("XOR A\n");
            this->a = 0;
            this->flags = 0x80; // this is always 0 to z flag is set, all else is 0
            fmt::print("\t\t\t\t\t\t\t\t\t a cleared\n");
            this->pc += 1;
            break;

        // INC / DEC
        case 0x05:
            fmt::print("DEC B\n");

            this->b -= 1;
            this->set_flag_z(this->b==0);
            this->set_flag_n(true);

            fmt::print("\t\t\t\t\t\t\t\t\t b decremented to ${:02X}\n", this->b);

            this->pc += 1;
            break;
        case 0x0C:
            fmt::print("INC C\n");

            this->c += 1;
            this->set_flag_z(this->c==0);
            this->set_flag_n(false);

            fmt::print("\t\t\t\t\t\t\t\t\t c incremented to ${:02X}\n", this->c);

            this->pc += 1;
            break;
        case 0x0D:
            fmt::print("DEC C\n");

            this->c -= 1;
            this->set_flag_z(this->c==0);
            this->set_flag_n(true);

            fmt::print("\t\t\t\t\t\t\t\t\t c decremented to ${:02X}\n", this->c);

            this->pc += 1;
            break;

        // Comparison
        case 0xFE: {
            fmt::print("CP d8\n");

            const auto val = bus.read(this->pc + 1);
            this->set_flag_z(this->a == val);
            this->set_flag_n(true);

            fmt::print("\t\t\t\t\t\t\t\t\t a (${:02X}) compared to ${:02X}\n", this->a, val);

            this->pc += 2;
            break;
        }

        // jumps
        case 0x20: {
            fmt::print("JR NZ, s8\n");
            int16_t offset = static_cast<int8_t>(bus.read(this->pc+1));
            this->pc += 2;
            if(!get_flag_z()) {
                this->pc += offset;
                fmt::print("\t\t\t\t\t\t\t\t\t relative jump taken with offset {} to ${:04X}\n", offset, this->pc);
            } else {
                fmt::print("\t\t\t\t\t\t\t\t\t relative jump NOT taken\n");
            }
            break;
        }
        case 0xC3: {
            fmt::print("JP nn nn\n");

            uint16_t addr_l = bus.read(this->pc + 1);
            uint16_t addr_h = bus.read(this->pc + 2);
            uint16_t new_pc = (addr_h << 8) | addr_l;
            fmt::print("\t\t\t\t\t\t\t\t\t Jumping to: ${:04X}\n", new_pc);

            this->pc = new_pc;
            break;
        }

        // interrupt stuff
        case 0xF3:
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
