#include "component.hpp"
#include "ser/store.hpp"
#include <filesystem>
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    BoardGraph b{"./assets/boards/board.json"};    
    std::cout << std::setw(4) << b.to_json() << std::endl;
    return 0;
}
