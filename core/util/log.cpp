#include "log.hpp"

#include <optional>
#include <memory>

std::optional<fmt::ostream> logger::_detail::log_stream{};
std::mutex logger::_detail::log_lock{};

void logger::init(const char * const fname) {
    logger::_detail::log_stream.emplace(fmt::output_file(fname));
}
