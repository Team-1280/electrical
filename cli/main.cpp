#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    
    json j;
    std::ifstream("../assets/components/test.json") >> j;
    model::Component a(j);
    model::Point b{model::Length(model::LengthUnit::Centimeters, 2), model::Length(model::LengthUnit::Feet, 3)};
    return 101;
}
