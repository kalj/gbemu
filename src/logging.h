#ifndef LOGGING_H
#define LOGGING_H

#include <fmt/core.h>
#include <string>

namespace logging {

    enum class LogLevel {
        QUIET,
        WARNING,
        DEBUG,
    };

    void set_level(LogLevel lvl);
    LogLevel get_level();

    static inline void debug(const std::string &s) {
        if (get_level() >= LogLevel::DEBUG) {
            fmt::print(s);
        }
    }

    static inline void warning(const std::string &s) {
        if (get_level() >= LogLevel::WARNING) {
            fmt::print(s);
        }
    }
} // namespace logging

#endif /* LOGGING_H */
