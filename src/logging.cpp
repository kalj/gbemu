#include "logging.h"

namespace logging {

    static LogLevel level{LogLevel::QUIET};

    void set_level(LogLevel lvl) {
        level = lvl;
    }

    LogLevel get_level() {
        return level;
    }
} // namespace logging
