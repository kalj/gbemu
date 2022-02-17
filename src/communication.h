#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <iosfwd>

class Communication {
public:
    uint8_t read_reg(uint8_t regid) const;
    void write_reg(uint8_t regid, uint8_t data);
    void dump(std::ostream &os) const;
private:
    uint8_t sb{0};
    uint8_t sc{0};
};

#endif /* COMMUNICATION_H */
