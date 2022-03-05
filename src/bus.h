#ifndef BUS_H
#define BUS_H

#include "ibus.h"

#include <cstdint>
#include <iosfwd>
#include <vector>

class InterruptState;
class Controller;
class Ppu;
class DivTimer;
class Sound;
class Communication;
class Cartridge;

class Bus : public IBus {
public:
    Bus(Cartridge &cart, Controller &cntl, Communication &comm, DivTimer &dt, Sound &snd, Ppu &ppu, InterruptState &is);

    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t data) override;
    void dump(std::ostream &os) const;

private:
    std::vector<uint8_t> vram;
    std::vector<uint8_t> wram;
    std::vector<uint8_t> hram;

    Cartridge &cartridge;
    Controller &controller;
    Communication &communication;
    DivTimer &div_timer;
    Sound &sound;
    Ppu &ppu;
    InterruptState &int_state;
};

#endif /* BUS_H */
