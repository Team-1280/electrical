#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    logger::error() << "Test" << std::endl;
    return 101;
}
