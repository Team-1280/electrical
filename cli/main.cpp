#include "component.hpp"
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    model::BoardGraph board{};
    
    //std::cout << board.to_json() << std::endl;
    return 0;
}
