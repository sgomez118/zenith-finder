#define NOMINMAX

// clang-format off
#include <optional>
#include "config_manager.hpp"
// clang-format on

#include <fstream>
#include <iostream>

namespace app {

Config ConfigManager::Load(const std::filesystem::path& path) {
  Config config;
  // Defaults
  config.observer = {51.5074, -0.1278, 0.0};
  config.catalog_path = "stars.csv";
  config.refresh_rate_ms = 1000;

  if (!std::filesystem::exists(path)) {
    return config;
  }

  try {
    toml::table tbl = toml::parse_file(path.string());

    if (tbl["observer"]["latitude"].is_number())
      config.observer.latitude = tbl["observer"]["latitude"].value_or(0.0);
    if (tbl["observer"]["longitude"].is_number())
      config.observer.longitude = tbl["observer"]["longitude"].value_or(0.0);
    if (tbl["observer"]["altitude"].is_number())
      config.observer.altitude = tbl["observer"]["altitude"].value_or(0.0);

    if (tbl["catalog"]["path"].is_string())
      config.catalog_path = tbl["catalog"]["path"].value_or("stars.csv");

    if (tbl["ui"]["refresh_rate_ms"].is_integer())
      config.refresh_rate_ms = tbl["ui"]["refresh_rate_ms"].value_or(1000);

  } catch (const toml::parse_error& err) {
    std::cerr << "Error parsing config file: " << err << "\n";
  }

  return config;
}

void ConfigManager::Save(const std::filesystem::path& path,
                         const Config& config) {
  auto tbl = toml::table{
      {"observer", toml::table{{"latitude", config.observer.latitude},
                               {"longitude", config.observer.longitude},
                               {"altitude", config.observer.altitude}}},
      {"catalog", toml::table{{"path", config.catalog_path}}},
      {"ui", toml::table{{"refresh_rate_ms", config.refresh_rate_ms}}}};

  std::ofstream file(path);
  if (file.is_open()) {
    file << tbl;
  }
}

}  // namespace app
