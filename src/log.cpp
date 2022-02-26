#include "log.h"

static bool log_enabled{false};

void log_set_enable(bool enabled) {
    log_enabled = enabled;
}

bool log_get_enable() {
    return log_enabled;
}
