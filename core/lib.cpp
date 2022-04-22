#include "util/log.hpp"
#include <cmath>
#include <iostream>
#include <lib.hpp>
#include <stdexcept>

namespace model {

void ComponentNode::from_json(ComponentNode &self, const json &j) {
    self.m_name = j["name"].get<std::string>();
    self.m_pos = j["pos"].get<Point>();
    self.m_ty = j[""]
}

}
