#include "config_manager.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace app {

Config ConfigManager::Load(const std::filesystem::path& path) {
  Config config;
  config.observer = {0.0, 0.0, 0.0};
  config.refresh_rate_ms = 1000;
  config.catalog_path = "stars.json";

  if (!std::filesystem::exists(path)) {
    return config;
  }

  try {
    auto data = toml::parse_file(path.string());
    if (auto obs = data["observer"].as_table()) {
      config.observer.latitude = (*obs)["latitude"].value_or(0.0);
      config.observer.longitude = (*obs)["longitude"].value_or(0.0);
      config.observer.altitude = (*obs)["altitude"].value_or(0.0);
    }
    config.catalog_path = data["catalog"]["path"].value_or("stars.json");
    config.ephemeris_path = data["ephemeris"]["path"].value_or("");
    config.refresh_rate_ms = data["app"]["refresh_rate_ms"].value_or(1000);
  } catch (const toml::parse_error& e) {
    std::cerr << "TOML Parsing Error: " << e.what() << std::endl;
  }

  return config;
}

void ConfigManager::Save(const std::filesystem::path& path,
                         const Config& config) {
  auto data = toml::table{
      {"observer",
       toml::table{{"latitude", config.observer.latitude},
                   {"longitude", config.observer.longitude},
                   {"altitude", config.observer.altitude}}},
      {"catalog", toml::table{{"path", config.catalog_path}}},
      {"ephemeris", toml::table{{"path", config.ephemeris_path}}},
      {"app", toml::table{{"refresh_rate_ms", config.refresh_rate_ms}}},
  };

  std::ofstream file(path);
  if (file.is_open()) {
    file << data;
  }
}

}  // namespace app
