#include "currency.hpp"
#include <doctest.h>

std::string USD::to_string() const {
    fmt::print("{:L}\n", this->m_dec);
    return fmt::format(
        std::locale("en_US.UTF-8"),
        "${:L}.{}",
        this->dollars(),
        this->cents()
    );
}

void USD::from_string(USD &self, const std::string_view str) {
    static auto parse_substr = [&str](std::size_t start, std::size_t end, const char* name) -> storage {
        storage val;
        auto [ptr, ec] = std::from_chars(str.data() + start, str.data() + end, val);
        if(ec != std::errc()) {
            throw std::runtime_error{fmt::format("String '{}' is not a valid USD amount (failed to convert {} to number)", str, name)};
        }
        return val;
    };

    self.m_dec = 0;
    if(str.empty()) throw std::runtime_error{"Empty string passed to USD#from_string"};
    std::size_t start_dollars = 0;
    const std::size_t len = str.length();
    std::size_t period_pos = str.find('.');
    period_pos = (period_pos == std::string_view::npos) ? len : period_pos;
    
    if(str.starts_with('$')) {
        start_dollars = 1;
        if(str.ends_with('c') && period_pos == len) {
            throw std::runtime_error{fmt::format("String {} begins with '$' and ends with 'c' without a decimal", str)};
        }
    }
    if(str.ends_with('c') && period_pos == len) {
        self.cents(parse_substr(0, len, "cents"));
        return;
    }

    self.dollars(parse_substr(start_dollars, period_pos, "dollars"));
    if(period_pos < len) {
        self.cents(parse_substr(period_pos + 1, len, "cents"));
    }
 
}

TEST_CASE("Currency") {
    USD six1{5, 100};
    CHECK_EQ(six1, USD{6});
    CHECK_EQ(six1 * 2, USD{12});
    SUBCASE("StringSerializable") {
        USD from_str{};
        USD::from_string(from_str, "$5.99");
        CHECK_EQ(from_str, USD{5, 99});
        USD::from_string(from_str, "40c");
        CHECK_EQ(from_str, USD{0, 40});
        USD::from_string(from_str, "40");
        CHECK_EQ(from_str, USD{40, 0});
        CHECK_THROWS(USD::from_string(from_str, "$40c"));
    }
}

