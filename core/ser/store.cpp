#include "store.hpp"
#include "util/log.hpp"

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
