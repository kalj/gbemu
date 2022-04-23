#ifndef LOGGING_H
#define LOGGING_H

#include <fmt/core.h>
#include <string>

#define LOGGING_ENABLED

namespace logging {

    enum class LogLevel {
        QUIET,
        WARNING,
        DEBUG,
    };

    void set_level(LogLevel lvl);
    LogLevel get_level();

    template <typename... Ts>
    static inline void debug(Ts... args) {
#ifdef LOGGING_ENABLED
        if (get_level() >= LogLevel::DEBUG) {
            fmt::print(args...);
        }
#endif
    }

    template <typename... T>
    static inline void warning(T... args) {
#ifdef LOGGING_ENABLED
        if (get_level() >= LogLevel::WARNING) {
            fmt::print(args...);
        }
#endif
    }
} // namespace logging

#endif /* LOGGING_H */
