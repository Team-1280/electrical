#include <iostream>
#include <lib.hpp>
#include <util/log.hpp>

#include "args.hpp"


int main(int argc, const char* argv[]) {
    logger::init("./log.txt");

    Args args{"e1280", "Electrical board creator"};

    auto help = args.arg(Arg {
        .takes_arg = false,
        .short_name{'h'},
        .long_name{"help"},
        .short_help{"Display a help message"}
    });
    
    try {
        auto matches = args.matches(argc, argv);
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Run e1280 --help for more information" << std::endl;
    }

    BoardGraph b{"./assets/boards/board.json"};    
    std::cout << std::setw(4) << b.to_json() << std::endl;

    return 0;
}
