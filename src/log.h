#ifndef LOG_H
#define LOG_H

#include <string>
#include <fmt/core.h>

void log_set_enable(bool enabled);
bool log_get_enable();

static inline void log(const std::string &s) {
    if(log_get_enable()) {
        fmt::print(s);
    }
}

#endif /* LOG_H */
