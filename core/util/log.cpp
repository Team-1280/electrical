#include "log.hpp"

#include <fstream>
#include <ios>
#include <memory>

std::unique_ptr<std::ostream> logger::_detail::log_stream{nullptr};
std::mutex logger::_detail::log_lock{};

void logger::init(const char * const fname) {
    logger::_detail::log_stream = std::make_unique<std::ofstream>(fname);
}

