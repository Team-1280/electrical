#include "log.hpp"

#include <fstream>
#include <ios>


void log::init(const char * const fname) {
    std::ofstream* of = new std::ofstream(fname, std::ofstream::out);
    log::_detail::log_stream = std::unique_ptr<std::ostream>(of);
}

