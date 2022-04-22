#include "component.hpp"
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    
    model::ComponentStore s{};
    std::cout << (*s.find("1280.test"))->to_json() << std::endl;
    return 0;
}
