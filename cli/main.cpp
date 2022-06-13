#include "component.hpp"
#include "ser/store.hpp"
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>

template<>
struct LazyResourceLoader<int> {
    static const std::filesystem::path RESOURCE_DIR;
    template<LazyResource... Args>
    static void load(Ref<int> a, const json& b, LazyResourceStore<Args...>& c) {

    }
};

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    Id id{"1280.test"};
    for(auto part : id) {
        std::cout << part << std::endl;
    }

    LazyResourceStore<int> a{};
    Ref<int> b = a.try_get<int>();
    
    //std::cout << board.to_json() << std::endl;
    return 0;
}
