#include "log.h"
#include <fmt/core.h>

static bool log_enabled{false};

void log_set_enable(bool enabled) {
    log_enabled = enabled;
}
void log(const std::string &s) {
    if(log_enabled) {
        fmt::print(s);
    }
}
