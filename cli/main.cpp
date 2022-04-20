#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    
    json j;
    std::ifstream("../assets/components/test.json") >> j;
    std::cout << model::Component(j).to_json() << std::endl;
    std::cout << model::Length("12 inches").to(model::LengthUnit::Feet).to_string() << std::endl;
    return 101;
}
