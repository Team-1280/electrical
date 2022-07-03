#include "store.hpp"
#include "util/log.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>

std::size_t TypeId::IDX = 0;

Id::Iterator& Id::Iterator::operator++() {
    this->m_pos += 1; 
    return *this;
}

bool Id::Iterator::operator==(Id::Iterator other) const {
    return this->m_pos == other.m_pos;
}

bool Id::Iterator::operator!=(Id::Iterator other) const {
    return !(*this == other);
}

Id::Iterator Id::Iterator::operator++(int) {
    Iterator me = *this;
    this->operator++();
    return me;
}

Id::Iterator::reference Id::Iterator::operator*() const {
    const char *data = this->m_super.m_string.data();
    const char *start = (this->m_pos == 0) ? data : data + this->m_super.m_dots[this->m_pos - 1] + 1;
    const char *next = (this->m_pos >= this->m_super.m_dots.size()) ? 
        this->m_super.m_string.cend().base() : 
        data + this->m_super.m_dots[this->m_pos];
    return std::string_view{start, static_cast<std::size_t>(next - start)};
}

Id::Iterator Id::begin() const {
    return Id::Iterator{*this, 0};
}

Id::Iterator Id::end() const {
    Id::Iterator id{*this, this->m_dots.size() + 1};
    return id;
}

void Id::to_path() {
    for(auto& idx : this->m_dots) {
        this->m_string.at(idx) = '/';
    }
}

void Id::to_id() {
    for(auto& idx : this->m_dots) {
        this->m_string.at(idx) = '.';
    }
}

static std::vector<std::size_t> find_dots(const std::string_view str) {
    std::vector<std::size_t> dots{};
    for(std::size_t pos = 0; const auto ch : str) {
        if(ch == '.') {
            dots.push_back(pos);
        }
        pos += 1;
    }
    return dots;
}

Id::Id(std::string&& str) : m_string{std::move(str)} {
    this->m_dots = find_dots(this->m_string);
}
Id::Id(const std::string& str) : m_string{str} {
    this->m_dots = find_dots(this->m_string);
}
Id::Id(const std::string_view str) : m_string{str} {
    this->m_dots = find_dots(this->m_string);
}


Ref<void> LazyResourceStore::try_get_id(TypeId type_id, const char *type_name, const std::string_view id_str) {
    auto elem = this->m_res.find(type_id.val());
    if(elem == this->m_res.end()) {
        throw UnregisteredResourceException(
            fmt::format("Type {} has no registered LazyResourceLoader implementation", type_name)
        );
    }
    
    auto cached = elem->second.cache.find(id_str);
    if(cached != elem->second.cache.end() && !cached->second.expired()) {
        return cached->second.lock();
    }
    
    try {
        Id id{id_str};
        id.to_path();
        std::filesystem::path resource_path = elem->second.loader->dir() / id.str();
        resource_path += ".json";

        logger::trace("Resource not found by ID, loading from {}", resource_path.c_str());
        std::ifstream file{};
        file.exceptions(std::ifstream::failbit);
        file.open(resource_path);
        json j;
        file >> j;
        
        auto [loaded, ins] = elem->second.cache.insert_or_assign(std::string{id_str}, WeakRef<void>{});
        Ref<void> load = elem->second.loader->load_untyped(loaded->first, j, *this);
        elem->second.cache.insert_or_assign(std::string{id_str}, WeakRef<void>{load});
        return load;
    } catch(const std::exception& e) {
        logger::error("Failed to deserialize element of type '{}' with id '{}': {}", type_name, id_str.data(), e.what());
        throw std::runtime_error(fmt::format("While loading '{}' with id '{}': {}", type_name, id_str.data(), e.what()));
    }
}
