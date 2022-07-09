#include "freelist.hpp"
#include <doctest.h>

TEST_CASE("FreeList") {
    FreeList<int> list{};
    auto first = list.emplace(5);
    list.emplace(14);
    list.erase(first);
    CHECK_MESSAGE(list.free_slots() == 1, "List should have 1 free slot after emplacing two items and erasing one");
    auto placed = list.emplace(12);
    CHECK_MESSAGE(placed == first, "List does not emplace items in empty slots");
}
