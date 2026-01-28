#include <CLI/CLI.hpp>
#include <iostream>
#include <format>
#include <vector>
#include "engine.hpp"

int main(int argc, char** argv) {
    CLI::App app{"Zenith Finder - Identify celestial objects at your local zenith"};

    engine::Observer obs{0.0, 0.0, 0.0};

    app.add_option("--lat", obs.latitude, "Observer latitude (degrees)")->required()->check(CLI::Range(-90.0, 90.0));
    app.add_option("--lon", obs.longitude, "Observer longitude (degrees)")->required()->check(CLI::Range(-180.0, 180.0));
    app.add_option("--alt", obs.altitude, "Observer altitude (meters)")->default_val(0.0);

    CLI11_PARSE(app, argc, argv);

    std::cout << std::format("Location: {:.4f}N, {:.4f}E, {:.1f}m\n", obs.latitude, obs.longitude, obs.altitude);
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << std::format("{:<20} | {:>10} | {:>10} | {:>10} | {:<10}\n", "Star Name", "Elevation", "Azimuth", "Zenith Dist", "Status");
    std::cout << "----------------------------------------------------------------------\n";

    auto results = engine::AstrometryEngine::calculate_zenith_proximity(obs);

    for (const auto& res : results) {
        std::string status = res.is_rising ? "[RISING]" : "[SETTING]";
        std::cout << std::format("{:<20} | {:>10.2f} | {:>10.2f} | {:>10.2f} | {:<10}\n", 
                                 res.name, res.elevation, res.azimuth, res.zenith_dist, status);
    }

    if (results.empty()) {
        std::cout << "No stars from the catalog are currently above the horizon.\n";
    }

    return 0;
}
