#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>

int main(int argc, const char* argv[]) {
    log::trace() << "Test" << std::endl;
    return 101;
}
