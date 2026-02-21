#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include "julian.hpp"

using namespace engine;
using namespace std::chrono;

TEST_CASE("Julian Date Clock Basic Operations", "[julian]") {
    // 1970-01-01 00:00:00 UTC is JD 2440587.5
    auto unix_epoch = sys_days{January/1/1970};
    auto jd_at_unix_epoch = clock_cast<JulianClock>(unix_epoch);
    
    // Convert to double days for comparison
    auto jd_days = duration_cast<duration<double, days::period>>(jd_at_unix_epoch.time_since_epoch());
    
    REQUIRE_THAT(jd_days.count(), 
                 Catch::Matchers::WithinAbs(2440587.5, 1e-9));

    SECTION("JD 0.0 is 4714 BC Nov 24 at noon") {
        auto jd0 = JulianClock::time_point{}; // Default constructor is epoch (JD 0)
        auto sys_at_jd0 = clock_cast<system_clock>(jd0);
        
        auto ymd = year_month_day{floor<days>(sys_at_jd0)};
        REQUIRE(ymd.year() == year{-4713});
        REQUIRE(ymd.month() == November);
        REQUIRE(ymd.day() == day{24});
        
        auto tod = sys_at_jd0 - floor<days>(sys_at_jd0);
        REQUIRE(tod == 12h);
    }

    SECTION("Round trip conversion") {
        auto now = system_clock::now();
        auto jd_now = clock_cast<JulianClock>(now);
        auto round_trip = clock_cast<system_clock>(jd_now);
        
        // Check for accuracy within 1ms
        auto diff = abs(duration_cast<milliseconds>(now - round_trip));
        REQUIRE(diff.count() <= 1);
    }
}

TEST_CASE("Julian Date Clock Precision and Overflow", "[julian]") {
    // Test that it handles high precision system_clock::now() which is often nanoseconds
    auto now = system_clock::now();
    auto jd_now = JulianClock::from_sys(now);
    
    // Check that we can convert it back
    auto round_trip = JulianClock::to_sys(jd_now);
    
    // Precision loss should be within 1 microsecond if it fell back to microseconds
    auto diff = abs(duration_cast<microseconds>(now - round_trip));
    REQUIRE(diff.count() <= 1);
}
