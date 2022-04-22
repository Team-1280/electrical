#pragma once

#include <memory>
#include <optional>
#include <mutex>
#include <fmt/format.h>
#include <cstdlib>

#include <buildopts.h>

namespace logger {

/**
 * @brief Initialize the global logger by creating a log file at fname
 * @param fname Path to the log file
 */
void init(const char * const fname);

/**
 * @brief Level of a recorded log message, used to 
 *
 */
enum class LogLevel: uint8_t {
    Trace,
    Warn,
    Error
};

namespace _detail {

extern std::unique_ptr<std::FILE> log_stream;
extern std::mutex log_lock;

template<LogLevel LVL>
struct lvl_data {
    static const char * const LVL_STR; 
};

template<> constexpr const char* const lvl_data<LogLevel::Error>::LVL_STR = "[ERROR] ";
template<> constexpr const char* const lvl_data<LogLevel::Warn>::LVL_STR = "[WARN] ";
template<> constexpr const char * const lvl_data<LogLevel::Trace>::LVL_STR = "[TRACE] ";

}


/**
 * @brief Get a thread-safe log buffer that will write messages to the global
 * output stream after all other write calls finish
 */ 
template<LogLevel lvl>
inline void log(fmt::string_view fmt, fmt::format_args args) {
    if (lvl != LogLevel::Trace || BuildOpts::should_log_trace()) {
        std::lock_guard lock{_detail::log_lock};
        fmt::print(_detail::log_stream.get(), _detail::lvl_data<lvl>::LVL_STR);
        fmt::vprint(_detail::log_stream.get(), fmt, args);
        fmt::print(_detail::log_stream.get(), "\n");
        std::fflush(_detail::log_stream.get());
    }
}

template<typename... Args>
inline void trace(fmt::format_string<Args...> fmt, Args&&... args) { log<LogLevel::Trace>(fmt, fmt::make_format_args(args...)); }
template<typename... Args>
inline void warn(fmt::format_string<Args...> fmt, Args&&... args) { log<LogLevel::Warn>(fmt, fmt::make_format_args(args...)); }
template<typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args&&... args) { log<LogLevel::Error>(fmt, fmt::make_format_args(args...)); }

}
