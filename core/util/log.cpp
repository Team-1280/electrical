#include "log.hpp"

#include <memory>

std::unique_ptr<std::FILE> logger::_detail::log_stream{};
std::mutex logger::_detail::log_lock{};

void logger::init(const char * const fname) {
    logger::_detail::log_stream = std::unique_ptr<std::FILE>{std::fopen(fname, "w")};
}
