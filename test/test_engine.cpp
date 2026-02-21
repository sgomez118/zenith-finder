#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <vector>

#include "engine.hpp"

using namespace engine;

TEST_CASE("Zenith Proximity Calculation Sanity Check", "[engine]") {
  // San Francisco coordinates
  Observer obs{37.7749, -122.4194, 0.0};

  auto now = std::chrono::system_clock::now();
  std::vector<Star> mock_catalog = {
      Star{.name = "Vega", .ra = 279.235, .dec = 38.784},
      Star{.name = "Sirius", .ra = 101.287, .dec = -16.716}};

  SECTION("Returns stars for valid input") {
    auto results =
        AstrometryEngine::CalculateZenithProximity(obs, mock_catalog, now);

    if (!results.empty()) {
      for (const auto& res : results) {
        REQUIRE(res.azimuth >= 0.0);
        REQUIRE(res.azimuth < 360.0);
        REQUIRE(res.elevation > 0.0);
        REQUIRE(res.elevation <= 90.0);
        REQUIRE_THAT(res.zenith_dist,
                     Catch::Matchers::WithinRel(90.0 - res.elevation, 0.001));
      }

      for (size_t i = 0; i < results.size() - 1; ++i) {
        REQUIRE(results[i].zenith_dist <= results[i + 1].zenith_dist);
      }
    }
  }

  SECTION("Coordinate Drift Test (Time-based)") {
    auto t1 = now;
    auto t2 = t1 + std::chrono::seconds(10);

    auto res1 =
        AstrometryEngine::CalculateZenithProximity(obs, mock_catalog, t1);
    auto res2 =
        AstrometryEngine::CalculateZenithProximity(obs, mock_catalog, t2);

    if (!res1.empty() && !res2.empty()) {
      REQUIRE(res1[0].azimuth != res2[0].azimuth);
    }
  }
}

TEST_CASE("Solar System Calculation", "[engine]") {
  Observer obs{0.0, 0.0, 0.0};
  auto now = std::chrono::system_clock::now();

  auto bodies = AstrometryEngine::CalculateSolarSystem(obs, now);

  // At least the Sun should be returned
  REQUIRE_FALSE(bodies.empty());

  for (const auto& body : bodies) {
    REQUIRE(body.name == "Sun");
    REQUIRE(body.distance_au > 0.9);  // Earth-Sun distance approx 1 AU
    REQUIRE(body.distance_au < 1.1);
  }
}
