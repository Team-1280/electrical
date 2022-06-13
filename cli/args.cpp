#include "args.hpp"

Arg::Id Args::arg(Arg &&arg) {
    this->m_args.push_back(std::move(arg));
    return this->m_args.size() - 1;
}
