#include <lib.hpp>

namespace model {

constexpr id FNV_PRIME = 1099511628211ULL;
constexpr id FNV_OFFSET = 14695981039346656037ULL;

/**
 * @brief Compute the FNV-1A hash of the given string
 */
constexpr id fnv1a(const std::string_view s) {
    id hash = FNV_OFFSET;
    for(char c : s) {
        hash = (hash * FNV_PRIME) ^ c;
    }
    return hash;
}

id Component::s_id_count = 0;

Component::Component(const json& val) {
    this->m_id = s_id_count;
    s_id_count += 1;

    val["name"].get_to(this->m_name);
}

}
