#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "engine.hpp"
#include "../app/location_provider.hpp"
#include <chrono>
#include <thread>

using namespace engine;

class MockLocationProvider : public app::LocationProvider {
public:
    MockLocationProvider(Observer start) : obs_(start) {}
    Observer GetLocation() override {
        // Simulate slight eastward drift (0.001 deg per call)
        obs_.longitude += 0.001;
        return obs_;
    }
private:
    Observer obs_;
};

TEST_CASE("Zenith Proximity Calculation Sanity Check", "[engine]") {
    // San Francisco coordinates
    Observer obs{37.7749, -122.4194, 0.0};
    
    auto now = std::chrono::system_clock::now();
    std::vector<Star> mock_catalog = {
        {"Vega", 279.235, 38.784},
        {"Sirius", 101.287, -16.716}
    };
    
    SECTION("Returns stars for valid input") {
        auto results = AstrometryEngine::CalculateZenithProximity(obs, mock_catalog, now);
        
        if (!results.empty()) {
            for (const auto& res : results) {
                REQUIRE(res.azimuth >= 0.0);
                REQUIRE(res.azimuth < 360.0);
                REQUIRE(res.elevation > 0.0);
                REQUIRE(res.elevation <= 90.0);
                REQUIRE_THAT(res.zenith_dist, Catch::Matchers::WithinRel(90.0 - res.elevation, 0.001));
            }
            
            for (size_t i = 0; i < results.size() - 1; ++i) {
                REQUIRE(results[i].zenith_dist <= results[i+1].zenith_dist);
            }
        }
    }

    SECTION("Coordinate Drift Test (Time-based)") {
        auto t1 = now;
        auto t2 = t1 + std::chrono::seconds(10);
        
        auto res1 = AstrometryEngine::CalculateZenithProximity(obs, mock_catalog, t1);
        auto res2 = AstrometryEngine::CalculateZenithProximity(obs, mock_catalog, t2);
        
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
        REQUIRE(body.distance_au > 0.9); // Earth-Sun distance approx 1 AU
        REQUIRE(body.distance_au < 1.1);
    }
}

TEST_CASE("Mock Location Provider", "[app]") {
    Observer start{0.0, 0.0, 0.0};
    MockLocationProvider mock(start);
    
    auto obs1 = mock.GetLocation();
    auto obs2 = mock.GetLocation();
    
    REQUIRE(obs2.longitude > obs1.longitude);
}
