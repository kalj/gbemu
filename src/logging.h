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

    template <typename... Args>
    static inline void debug(fmt::format_string<Args...> s, Args&&... args) {
#ifdef LOGGING_ENABLED
        if (get_level() >= LogLevel::DEBUG) {
            fmt::print(s, std::forward<Args>(args)...);
        }
#endif
    }

    template <typename... Args>
    static inline void warning(fmt::format_string<Args...> s, Args&&... args) {
#ifdef LOGGING_ENABLED
        if (get_level() >= LogLevel::WARNING) {
            fmt::print(s, std::forward<Args>(args)...);
        }
#endif
    }
} // namespace logging

#endif /* LOGGING_H */
