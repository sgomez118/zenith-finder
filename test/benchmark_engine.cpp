#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <atomic>

#include "engine.hpp"

using namespace engine;

// Allocation tracking
static std::atomic<size_t> g_alloc_count{0};
static std::atomic<size_t> g_alloc_bytes{0};
static bool g_track_allocs = false;

void* operator new(std::size_t size) {
    if (g_track_allocs) {
        g_alloc_count++;
        g_alloc_bytes += size;
    }
    return std::malloc(size);
}

void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    std::free(ptr);
}

namespace {

std::vector<Star> GenerateMockCatalog(size_t count) {
    std::vector<Star> catalog;
    catalog.reserve(count);

    std::mt19937 gen(42);  // Fixed seed
    std::uniform_real_distribution<double> ra_dist(0.0, 360.0);
    std::uniform_real_distribution<double> dec_dist(-90.0, 90.0);
    std::uniform_real_distribution<float> mag_dist(-1.5f, 10.0f);

    for (size_t i = 0; i < count; ++i) {
        catalog.push_back(Star{.name = "Star " + std::to_string(i),
                               .catalog = "MOCK",
                               .catalog_id = static_cast<long>(i),
                               .ra = ra_dist(gen),
                               .dec = dec_dist(gen),
                               .pmra = 0.0,
                               .pmdec = 0.0,
                               .parallax = 0.0,
                               .radial_velocity = 0.0,
                               .flux = mag_dist(gen)});
    }
    return catalog;
}

struct BenchResult {
    size_t count;
    long long set_catalog_ms;
    long long calculate_ms;
    size_t result_count;
    size_t allocations;
    size_t allocated_kb;
};

BenchResult RunBenchmark(AstrometryEngine& engine, ResultBuffer& buffer, size_t count, const Observer& obs,
                        std::chrono::system_clock::time_point time) {
    auto catalog = GenerateMockCatalog(count);

    // Measure SetCatalog
    auto start_set = std::chrono::high_resolution_clock::now();
    engine.SetCatalog(catalog);
    auto end_set = std::chrono::high_resolution_clock::now();

    // Reset allocation counters and start tracking
    g_alloc_count = 0;
    g_alloc_bytes = 0;
    g_track_allocs = true;

    // Measure CalculateZenithProximity (Buffered)
    auto start_calc = std::chrono::high_resolution_clock::now();
    engine.CalculateZenithProximity(buffer, obs, {}, {}, time);
    auto end_calc = std::chrono::high_resolution_clock::now();

    g_track_allocs = false;

    return BenchResult{
        .count = count,
        .set_catalog_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_set - start_set).count(),
        .calculate_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_calc - start_calc).count(),
        .result_count = buffer.star_results.size(),
        .allocations = g_alloc_count.load(),
        .allocated_kb = g_alloc_bytes.load() / 1024
    };
}

}  // namespace

TEST_CASE("Engine Performance Benchmarking", "[.benchmark]") {
    Observer obs{37.7749, -122.4194, 0.0};  // San Francisco
    auto now = std::chrono::system_clock::now();

    AstrometryEngine engine;
    ResultBuffer buffer;
    
    // Warm up
    {
        auto warm_catalog = GenerateMockCatalog(100);
        engine.SetCatalog(warm_catalog);
        engine.CalculateZenithProximity(buffer, obs, {}, {}, now);
    }

    SECTION("Scaling Tests") {
        std::vector<size_t> sizes = {5000, 10000, 50000};
        
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << " ENGINE PERFORMANCE BENCHMARKS (BUFFERED OVERLOAD)\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << std::left << std::setw(10) << "Stars" 
                  << std::setw(15) << "Set (ms)" 
                  << std::setw(15) << "Calc (ms)" 
                  << std::setw(12) << "Results" 
                  << std::setw(12) << "Allocs" 
                  << std::setw(12) << "Memory (KB)" << "\n";
        std::cout << std::string(80, '-') << "\n";

        for (size_t size : sizes) {
            auto res = RunBenchmark(engine, buffer, size, obs, now);
            std::cout << std::left << std::setw(10) << res.count
                      << std::setw(15) << res.set_catalog_ms
                      << std::setw(15) << res.calculate_ms
                      << std::setw(12) << res.result_count
                      << std::setw(12) << res.allocations
                      << std::setw(12) << res.allocated_kb << "\n";
        }
        std::cout << "\n* Note: 'Memory' includes internal parallel intermediate buffers.\n";
        std::cout << std::string(80, '=') << "\n" << std::endl;
    }
}
