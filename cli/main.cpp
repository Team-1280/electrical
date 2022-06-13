#include <iostream>
#include <lib.hpp>
#include <util/log.hpp>

#include "args.hpp"


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");

    Args args{};

    BoardGraph b{"./assets/boards/board.json"};    
    std::cout << std::setw(4) << b.to_json() << std::endl;

    return 0;
}
