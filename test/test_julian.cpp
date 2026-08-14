#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>

#include "julian.hpp"

namespace engine {
namespace {

TEST_CASE("Julian Date Clock Basic Operations", "[julian]") {
  using namespace std::chrono_literals;
  // 1970-01-01 00:00:00 UTC is JD 2440587.5
  auto unix_epoch = std::chrono::sys_days{std::chrono::January / 1 / 1970};
  auto jd_at_unix_epoch = std::chrono::clock_cast<JulianClock>(unix_epoch);

  // Convert to double days for comparison
  auto jd_days = std::chrono::duration_cast<
      std::chrono::duration<double, std::chrono::days::period>>(
      jd_at_unix_epoch.time_since_epoch());

  REQUIRE_THAT(jd_days.count(), Catch::Matchers::WithinAbs(2440587.5, 1e-9));

  SECTION("JD 0.0 is 4714 BC Nov 24 at noon") {
    auto jd0 =
        JulianClock::time_point{};  // Default constructor is epoch (JD 0)
    auto sys_at_jd0 = std::chrono::clock_cast<std::chrono::system_clock>(jd0);

    auto ymd = std::chrono::year_month_day{
        std::chrono::floor<std::chrono::days>(sys_at_jd0)};
    REQUIRE(ymd.year() == std::chrono::year{-4713});
    REQUIRE(ymd.month() == std::chrono::November);
    REQUIRE(ymd.day() == std::chrono::day{24});

    auto tod = sys_at_jd0 - std::chrono::floor<std::chrono::days>(sys_at_jd0);
    REQUIRE(tod == 12h);
  }

  SECTION("Round trip conversion") {
    auto now = std::chrono::system_clock::now();
    auto jd_now = std::chrono::clock_cast<JulianClock>(now);
    auto round_trip =
        std::chrono::clock_cast<std::chrono::system_clock>(jd_now);

    // Check for accuracy within 1ms
    auto diff = std::abs(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - round_trip)
            .count());
    REQUIRE(diff <= 1);
  }
}

TEST_CASE("Julian Day Parts Extraction", "[julian]") {
  using namespace std::chrono_literals;
  // 2024-05-18 18:00:00 UTC
  // JD = 2460449.25
  auto tp = std::chrono::sys_days{std::chrono::May / 18 / 2024} + 18h;

  SECTION("Using system_clock") {
    auto parts = GetJulianDayParts(tp);
    REQUIRE(parts.day_number == 2460449);
    REQUIRE_THAT(parts.fraction, Catch::Matchers::WithinAbs(0.25, 1e-9));
  }

  SECTION("Using JulianClock time_point") {
    auto jd_tp = std::chrono::clock_cast<JulianClock>(tp);
    auto parts = GetJulianDayParts(jd_tp);
    REQUIRE(parts.day_number == 2460449);
    REQUIRE_THAT(parts.fraction, Catch::Matchers::WithinAbs(0.25, 1e-9));
  }
}

TEST_CASE("Julian Date Clock Precision and Overflow", "[julian]") {
  // Test that it handles high precision system_clock::now() which is often
  // nanoseconds
  auto now = std::chrono::system_clock::now();
  auto jd_now = JulianClock::from_sys(now);

  // Check that we can convert it back
  auto round_trip = JulianClock::to_sys(jd_now);

  // Precision loss should be within 1 microsecond if it fell back to
  // microseconds
  auto diff = std::abs(
      std::chrono::duration_cast<std::chrono::microseconds>(now - round_trip)
          .count());
  REQUIRE(diff <= 1);
}

}  // namespace
}  // namespace engine
