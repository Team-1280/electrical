#pragma once

#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <mutex>
#include <sstream>
#include <string_view>
#include <functional>
#include <iomanip>
#include <concepts>
#include <fmt/format.h>
#include <fmt/os.h>

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

extern std::optional<fmt::ostream> log_stream;
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
template<LogLevel lvl, typename... Args>
inline void log(fmt::format_string<Args...> fmt, Args&&... args) {
    if constexpr(lvl != LogLevel::Trace || BuildOpts::should_log_trace()) {
        if(!_detail::log_stream.has_value()) return;
        std::lock_guard lock{_detail::log_lock};
        _detail::log_stream->print(_detail::lvl_data<lvl>::LVL_STR);
        _detail::log_stream->print(fmt, std::forward(args)...);
    }
}

template<typename... Args>
inline void trace(fmt::format_string<Args...> fmt, Args&&... args) { log<LogLevel::Trace>(fmt, std::forward(args)...); }
template<typename... Args>
inline void warn(fmt::format_string<Args...> fmt, Args&&... args) { log<LogLevel::Trace>(fmt, std::forward(args)...); }
template<typename... Args>
inline auto error(fmt::format_string<Args...> fmt, Args&&... args) { log<LogLevel::Trace>(fmt, std::forward(args)...); }

}
