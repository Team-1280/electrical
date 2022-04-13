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
    static constexpr const char * const LVL_STR = "";
public:
    LogBuf() = default;
    LogBuf(const LogBuf&) = delete;
    LogBuf& operator=(const LogBuf&) = delete;
    LogBuf& operator=(LogBuf&&) = delete;
    LogBuf(LogBuf&& other) : ss(std::move(other.ss)) {}

    template<typename T> LogBuf& operator<<(const T& msg) {
        ss << msg;
        return *this;
    }

    LogBuf& operator<<(std::ostream& (*manip)(std::ostream&)) {
	    manip(ss);
	    return *this;
    }

    ~LogBuf() {
        std::lock_guard<std::mutex> lock{log_lock};
        *log_stream << LVL_STR << ": " << ss.rdbuf() << std::endl;
    }
private:
    std::stringstream ss;
};

template<> constexpr const char* const LogBuf<LogLevel::Error>::LVL_STR = "[ERROR]";
template<> constexpr const char* const LogBuf<LogLevel::Warn>::LVL_STR = "[WARN]";
template<> constexpr const char * const LogBuf<LogLevel::Trace>::LVL_STR = "[TRACE]";

#ifdef DISABLE_TRACE
template<> class LogBuf<LogLevel::Trace> final {
    template<typename T> constexpr LogBuf& operator<<(T&& msg) { return *this; }
    LogBuf& operator<<(std::ostream& (*manip)(std::ostream&)) { return *this; }
    ~LogBuf() {}
};
#endif

}


/**
 * @brief Get a thread-safe log buffer that will write messages to the global
 * output stream after all other write calls finish
 */ 
template<LogLevel lvl> constexpr inline _detail::LogBuf<lvl> log() { return _detail::LogBuf<lvl>{}; }

inline auto trace() { return _detail::LogBuf<LogLevel::Trace>{}; }
inline auto warn() { return _detail::LogBuf<LogLevel::Warn>{}; }
inline auto error() { return _detail::LogBuf<LogLevel::Error>{}; }

}
