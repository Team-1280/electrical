#include "component.hpp"
#include "ser/store.hpp"
#include <filesystem>
#include <lib.hpp>
#include <util/log.hpp>
#include <iostream>
#include <fstream>

class IntResourceLoader : public LazyResourceLoader<int> {
public:
    IntResourceLoader() {}
    ~IntResourceLoader() = default;

    Ref<int> load(Id&& id, const json& j, LazyResourceStore& store) {
        Ref<int> i{new int{j.get<int>()}};
        return i;
    }

    std::filesystem::path const& dir() const noexcept {
        return DIR;
    }

private:
    static std::filesystem::path DIR;
};
std::filesystem::path IntResourceLoader::DIR = "./assets/connectors";

int main(int argc, const char* argv[]) {
    logger::init("./log.txt");
    Id id{std::string_view{"1280.test"}};
    for(auto part : id) {
        std::cout << part << std::endl;
    }

    LazyResourceStore a{};
    a.register_loader(new IntResourceLoader{});
    Ref<int> b = a.try_get<int>("1280/test-conn");
    
    std::cout << *b << std::endl;
    return 0;
}
