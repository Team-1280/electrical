#pragma once

#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <mutex>
#include <sstream>
#include <string_view>
#include <utility>

namespace log {

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

static std::unique_ptr<std::ostream> log_stream;
static std::mutex log_lock{};

/**
 * @brief A stream that stores written data
 * and writes it to the global log file in a thread-safe way upon destruction
 */
template<LogLevel lvl> class LogBuf final {
public:
    LogBuf() = default;
    LogBuf(const LogBuf&) = delete;
    LogBuf& operator=(const LogBuf&) = delete;
    LogBuf& operator=(LogBuf&&) = delete;
    LogBuf(LogBuf&& other) : ss(std::move(other.ss)) {}

    template<typename T> LogBuf& operator<<(T&& msg) {
        this->ss << std::forward(msg);
        return *this;
    }

    LogBuf& operator<<(std::ostream& (*manip)(std::ostream&)) {
	    manip(this->ss);
	    return *this;
    }

    ~LogBuf() {
        std::lock_guard<std::mutex> guard{log_lock};
        *log_stream << ss.rdbuf();
    }

private:
    std::stringstream ss; 
};

#ifdef DISABLE_TRACE
template<> class LogBuf<LogLevel::Trace> final {
    template<typename T> constexpr LogBuf& operator<<(T&& msg) { return *this; }
    LogBuf& operator<<(std::ostream& (*manip)(std::ostream&)) { return *this; }
};
#endif

}


/**
 * @brief Get a thread-safe log buffer that will write messages to the global
 * output stream after all other write calls finish
 */ 
template<LogLevel lvl> constexpr inline _detail::LogBuf<lvl> log() { return _detail::LogBuf<lvl>{}; }

inline auto trace() { return _detail::LogBuf<LogLevel::Trace>(); }
inline auto warn() { return _detail::LogBuf<LogLevel::Warn>{}; }
inline auto error() { return _detail::LogBuf<LogLevel::Error>{}; }

}
