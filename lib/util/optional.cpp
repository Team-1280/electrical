#include "optional.hpp"
#include <doctest.h>

struct NoneableMock {
    NoneableMock() = default;
    NoneableMock(NoneableMock&&) = default;
    NoneableMock(bool n) : none{n} {}

    inline constexpr bool is_none() const noexcept { return this->none; }
    inline constexpr void make_none() noexcept { this->none = true; }

    bool none{false};
};

static_assert(Noneable<NoneableMock>);

TEST_CASE("Optional") {
    CHECK_MESSAGE(sizeof(Optional<NoneableMock>) == sizeof(NoneableMock), "Optional<Noneable T> optimization not applied");
    Optional<int> opt{5};
    SUBCASE("equality") {
        Optional<int> other_opt{5};
        CHECK(opt == 5);
        CHECK(opt != 12);
        CHECK(opt == other_opt);
    }
    SUBCASE("unwrap") {
        opt.reset();
        CHECK_THROWS(opt.unwrap_except(""));
        CHECK_EQ(opt.unwrap_or(5), 5);
    }
    SUBCASE("has_value") {
        opt.emplace(12);
        CHECK(opt.has_value());
        CHECK(opt);
    }
    SUBCASE("map") {
        opt.emplace(15);
        CHECK(opt.map([](int const& v) { return v + 20; }) == 35);
    }
}
