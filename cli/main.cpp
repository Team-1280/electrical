#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    
    json j;
    std::ifstream("../assets/components/test.json") >> j;
    model::Component a(j);
    return 101;
}
