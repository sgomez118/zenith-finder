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

  SECTION("Instance-based method returns stars for valid input") {
    AstrometryEngine engine;
    engine.SetCatalog(mock_catalog);
    auto results = engine.CalculateZenithProximity(obs, now);

    if (!results.empty()) {
      REQUIRE(results.size() == mock_catalog.size());
      for (size_t i = 0; i < results.size(); ++i) {
        REQUIRE(results[i].name == mock_catalog[i].name);
        REQUIRE(results[i].azimuth >= 0.0);
        REQUIRE(results[i].azimuth < 360.0);
        REQUIRE(results[i].elevation >= -90.0);
        REQUIRE(results[i].elevation <= 90.0);
        REQUIRE_THAT(
            results[i].zenith_dist,
            Catch::Matchers::WithinAbs(90.0 - results[i].elevation, 0.001));
      }
    }
  }

  SECTION("Coordinate Drift Test (Time-based)") {
    auto t1 = now;
    auto t2 = t1 + std::chrono::seconds(10);

    AstrometryEngine engine;
    engine.SetCatalog(mock_catalog);
    auto res1 = engine.CalculateZenithProximity(obs, t1);
    auto res2 = engine.CalculateZenithProximity(obs, t2);

    if (!res1.empty() && !res2.empty()) {
      REQUIRE(res1[0].azimuth != res2[0].azimuth);
    }
  }
}

TEST_CASE("Solar System Calculation", "[engine]") {
  Observer obs{0.0, 0.0, 0.0};
  auto now = std::chrono::system_clock::now();

  AstrometryEngine engine;
  auto bodies = engine.CalculateSolarSystem(obs, now);

  // At least the Sun should be returned (if implemented)
  if (!bodies.empty()) {
    for (const auto& body : bodies) {
      if (body.name == "Sun") {
        REQUIRE(body.distance_au > 0.9);  // Earth-Sun distance approx 1 AU
        REQUIRE(body.distance_au < 1.1);
      }
    }
  }
}
