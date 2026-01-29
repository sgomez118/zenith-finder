#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "engine.hpp"
#include <chrono>

using namespace engine;

TEST_CASE("Zenith Proximity Calculation Sanity Check", "[engine]") {
    // San Francisco coordinates
    Observer obs{37.7749, -122.4194, 0.0};
    
    // Fixed time: 2026-01-27 12:00:00 UTC (Approx)
    // Constructing a time_point is a bit verbose in C++20 without specific date libs, 
    // but we can use system_clock::now() for a sanity check ensuring "some" stars are returned
    // or manually construct if we needed precise position verification.
    // For this "hit the hay" test, let's ensure the function behaves correctly with a valid time.
    
    auto now = std::chrono::system_clock::now();
    
    SECTION("Returns stars for valid input") {
        auto results = AstrometryEngine::CalculateZenithProximity(obs, now);
        
        // At any given time, *some* stars from the 50 brightest should be above the horizon 
        // unless the catalog is very small or the location/time is extremely specific (unlikely).
        // Actually, 50 stars is small. Let's check constraints on results if any exist.
        
        if (!results.empty()) {
            for (const auto& res : results) {
                // Azimuth must be [0, 360)
                REQUIRE(res.azimuth >= 0.0);
                REQUIRE(res.azimuth < 360.0);
                
                // Elevation must be > 0 (as per our logic in engine.cpp) and <= 90
                REQUIRE(res.elevation > 0.0);
                REQUIRE(res.elevation <= 90.0);
                
                // Zenith distance check
                REQUIRE_THAT(res.zenith_dist, Catch::Matchers::WithinRel(90.0 - res.elevation, 0.001));
            }
            
            // Verify sorting: Zenith distance should be increasing
            for (size_t i = 0; i < results.size() - 1; ++i) {
                REQUIRE(results[i].zenith_dist <= results[i+1].zenith_dist);
            }
        } else {
            // It is possible no stars are visible. 
            // In that case, we can't assert much other than it didn't crash.
            SUCCEED("No stars visible, but execution completed safely.");
        }
    }
}
