#pragma once

#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <mutex>
#include <sstream>
#include <string_view>
#include <functional>
#include <iomanip>

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

extern std::unique_ptr<std::ostream> log_stream;
extern std::mutex log_lock;

/**
 * @brief A stream that stores written data
 * and writes it to the global log file in a thread-safe way upon destruction
 */
template<LogLevel LVL> class LogBuf final {
    static const char * const LVL_STR;
public:
    LogBuf() = default;
    LogBuf(const LogBuf&) = delete;
    LogBuf& operator=(const LogBuf&) = delete;
    LogBuf& operator=(LogBuf&&) = delete;
    LogBuf(LogBuf&& other) : ss(std::move(other.ss)) {}

    template<typename T> inline LogBuf& operator<<(const T& msg) {
        ss << msg;
        return *this;
    }

    inline LogBuf& operator<<(std::ostream& (*manip)(std::ostream&)) {
	    manip(ss);
	    return *this;
    }

    ~LogBuf() {
        std::lock_guard<std::mutex> lock{log_lock};
        (*log_stream) << LVL_STR << ": " << ss.rdbuf() << std::endl;
    }
private:
    std::stringstream ss;
};

template<LogLevel LVL> class DummyBuf final {
public:
    template<typename T>
    inline constexpr DummyBuf& operator<<(const T&) { return *this; }
};

template<> constexpr const char* const LogBuf<LogLevel::Error>::LVL_STR = "[ERROR]";
template<> constexpr const char* const LogBuf<LogLevel::Warn>::LVL_STR = "[WARN]";
template<> constexpr const char * const LogBuf<LogLevel::Trace>::LVL_STR = "[TRACE]";

}


/**
 * @brief Get a thread-safe log buffer that will write messages to the global
 * output stream after all other write calls finish
 */ 
template<LogLevel lvl> constexpr inline _detail::LogBuf<lvl> log() { 
    if constexpr(lvl == LogLevel::Trace && !BuildOpts::should_log_trace()) {
        return _detail::DummyBuf<lvl>{};
    } else {
        return _detail::LogBuf<lvl>{}; 
    }
}

inline auto trace() { return _detail::LogBuf<LogLevel::Trace>{}; }
inline auto warn() { return _detail::LogBuf<LogLevel::Warn>{}; }
inline auto error() { return _detail::LogBuf<LogLevel::Error>{}; }

}
