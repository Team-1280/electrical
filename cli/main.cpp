#include "component.hpp"
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");

    json board_json;
    std::ifstream{"./assets/boards/test.json"} >> board_json;
    model::BoardGraph board = board_json.get<model::BoardGraph>();
    
    std::cout << board.to_json() << std::endl;
    return 0;
}
