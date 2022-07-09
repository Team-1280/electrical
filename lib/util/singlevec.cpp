#include "singlevec.hpp"
#include <doctest.h>

TEST_CASE("SingleVec") {
    SingleVec<int> vec{5};

    SUBCASE("Removal operations preserve one element") {
        vec.pop_back();
        CHECK(vec.size() == 1);
    }
}
