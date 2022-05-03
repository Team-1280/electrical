#include "component.hpp"
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    model::BoardGraph board{};
    Ref<model::Connector> conn = *board.get<model::Connector>("1280.bare");
    
    //std::cout << board.to_json() << std::endl;
    return 0;
}
