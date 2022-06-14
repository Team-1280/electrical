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

    auto version = args.arg(Arg {
        .takes_arg = false,
        .short_name{'v'},
        .long_name{"version"},
        .short_help{"Display program version message"}
    });
    
    Args list_ids{"list", "List all node and edge IDs in the graph"};
    auto show_all = list_ids.arg(Arg {
        .takes_arg = false,
        .short_name{'a'},
        .long_name{"all-id"},
        .short_help{"Display all IDs"}
    });

    args.command(std::move(list_ids));
    
    try {
        auto matches = args.matches(argc, argv);
        if(matches.has(help)) {
            args.print_help();
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Run e1280 --help for more information" << std::endl;
    }

    BoardGraph b{"./assets/boards/board.json"};    
    std::cout << std::setw(4) << b.to_json() << std::endl;

    return 0;
}
