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
    auto results = engine.CalculateZenithProximity(obs, {}, {}, now);

    // Some stars might be below horizon depending on the time of day
    REQUIRE(results.size() <= mock_catalog.size());
    for (const auto& result : results) {
      REQUIRE(result.azimuth >= 0.0);
      REQUIRE(result.azimuth < 360.0);
      REQUIRE(result.elevation >= 0.0);  // Now that we filter
      REQUIRE(result.elevation <= 90.0);
      REQUIRE_THAT(result.zenith_dist,
                   Catch::Matchers::WithinAbs(90.0 - result.elevation, 0.001));
    }
  }

  SECTION("Rising/Setting trend check") {
    AstrometryEngine engine;
    engine.SetCatalog(mock_catalog);
    auto res = engine.CalculateZenithProximity(obs, {}, {}, now);

    if (!res.empty()) {
      auto t_future = now + std::chrono::seconds(10);
      auto res_future = engine.CalculateZenithProximity(obs, {}, {}, t_future);

      if (!res_future.empty()) {
        for (size_t i = 0; i < res.size(); ++i) {
          // Find the same star in res_future (might be skipped if it set)
          auto it = std::find_if(
              res_future.begin(), res_future.end(),
              [&](const CelestialResult& r) { return r.name == res[i].name; });
          if (it != res_future.end()) {
            bool actually_rising = it->elevation > res[i].elevation;
            REQUIRE(res[i].is_rising == actually_rising);
          }
        }
      }
    }
  }

  SECTION("Filtering in engine") {
    AstrometryEngine engine;
    engine.SetCatalog(mock_catalog);

    FilterCriteria filter;
    filter.active = true;
    filter.name_filter = "Vega";

    auto results = engine.CalculateZenithProximity(obs, filter, {}, now);
    for (const auto& res : results) {
      REQUIRE(res.name == "Vega");
    }

    filter.name_filter = "NON_EXISTENT_STAR";
    results = engine.CalculateZenithProximity(obs, filter, {}, now);
    REQUIRE(results.empty());
  }

  SECTION("Offset and Limit in engine") {
    AstrometryEngine engine;
    std::vector<Star> many_stars;
    for (int i = 0; i < 10; ++i) {
      many_stars.push_back(Star{.name = "Star " + std::to_string(i),
                                .ra = static_cast<double>(i),
                                .dec = 0.0});
    }
    engine.SetCatalog(many_stars);

    FilterCriteria filter;
    filter.active = true;
    filter.min_elevation = -90.0f;  // Ensure all stars are "visible" for test

    SECTION("Limit only") {
      filter.limit = 5;
      auto results = engine.CalculateZenithProximity(obs, filter, {}, now);
      REQUIRE(results.size() == 5);
    }

    SECTION("Offset only") {
      filter.offset = 7;
      auto results = engine.CalculateZenithProximity(obs, filter, {}, now);
      REQUIRE(results.size() == 3);
    }

    SECTION("Offset and Limit") {
      filter.offset = 2;
      filter.limit = 3;
      auto results = engine.CalculateZenithProximity(obs, filter, {}, now);
      REQUIRE(results.size() == 3);
      REQUIRE(results[0].name == "Star 2");
      REQUIRE(results[2].name == "Star 4");
    }

    SECTION("Offset out of bounds") {
      filter.offset = 20;
      auto results = engine.CalculateZenithProximity(obs, filter, {}, now);
      REQUIRE(results.empty());
    }
  }

  SECTION("Sorting in engine") {
    AstrometryEngine engine;
    engine.SetCatalog(mock_catalog);

    SortCriteria sort;
    sort.column = SortColumn::NAME;
    sort.ascending = true;

    auto results = engine.CalculateZenithProximity(obs, {}, sort, now);
    if (results.size() >= 2) {
      for (size_t i = 0; i < results.size() - 1; ++i) {
        REQUIRE(results[i].name <= results[i + 1].name);
      }
    }

    sort.ascending = false;
    results = engine.CalculateZenithProximity(obs, {}, sort, now);
    if (results.size() >= 2) {
      for (size_t i = 0; i < results.size() - 1; ++i) {
        REQUIRE(results[i].name >= results[i + 1].name);
      }
    }
  }
}

TEST_CASE("Solar System Calculation", "[engine]") {
  Observer obs{0.0, 0.0, 0.0};
  auto now = std::chrono::system_clock::now();

  AstrometryEngine engine;
  auto bodies = engine.CalculateSolarSystem(obs, {}, {}, now);

  // At least the Sun should be returned (if implemented)
  if (!bodies.empty()) {
    for (const auto& body : bodies) {
      if (body.name == "SUN") {
        REQUIRE(body.distance_au > 0.9);  // Earth-Sun distance approx 1 AU
        REQUIRE(body.distance_au < 1.1);
      }
    }
  }
}
