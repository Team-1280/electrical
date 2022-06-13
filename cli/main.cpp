#include "component.hpp"
#include "ser/store.hpp"
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    Id id{"1280.test"};
    for(auto part : id) {
        std::cout << part << std::endl;
    }
    
    //std::cout << board.to_json() << std::endl;
    return 0;
}
