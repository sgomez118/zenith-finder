#include "catalog_loader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace engine {

std::vector<Star> CatalogLoader::LoadStarDataFromCSV(
    const std::filesystem::path& path) {
  std::vector<Star> catalog;
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open star catalog " << path << std::endl;
    return catalog;
  }

  std::string line;
  // Skip header
  std::getline(file, line);

  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string field;
    Star star;

    std::getline(ss, star.name, ',');
    std::getline(ss, star.catalog, ',');
    std::getline(ss, field, ',');
    star.catalog_id = std::stol(field);
    std::getline(ss, field, ',');
    star.ra = std::stod(field);
    std::getline(ss, field, ',');
    star.dec = std::stod(field);
    // ... parse other fields if needed ...

    catalog.push_back(star);
  }

  return catalog;
}

std::vector<Star> CatalogLoader::LoadStarDataFromJSON(
    const std::filesystem::path& path) {
  std::vector<Star> catalog;
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open star catalog " << path << std::endl;
    return catalog;
  }

  try {
    nlohmann::json j;
    file >> j;

    if (!j.contains("data") || !j["data"].is_array()) {
      return catalog;
    }

    for (const auto& item : j["data"]) {
      if (!item.is_array() || item.size() < 15) {
        continue;
      }

      Star star;
      // Positional fields from JSON
      // 0: main_id, 1: ra, 2: dec, 3: coo_qual, 4: pmra, 5: pmdec, 6: pm_qual,
      // 7: plx, 8: plx_qual, 9: rv, 10: rv_qual, 11: flux, 12: flux_err,
      // 13: flux_qual, 14: ids
      star.ra = item[1].get<double>();
      star.dec = item[2].get<double>();
      star.coo_qual = item[3].is_string() ? item[3].get<std::string>()[0] : ' ';
      star.pmra = item[4].is_number() ? item[4].get<double>() : 0.0;
      star.pmdec = item[5].is_number() ? item[5].get<double>() : 0.0;
      star.pm_qual = item[6].is_string() ? item[6].get<std::string>()[0] : ' ';
      star.parallax = item[7].is_number() ? item[7].get<double>() : 0.0;
      star.plx_qual = item[8].is_string() ? item[8].get<std::string>()[0] : ' ';
      star.radial_velocity = item[9].is_number() ? item[9].get<double>() : 0.0;
      star.rvz_qual =
          item[10].is_string() ? item[10].get<std::string>()[0] : ' ';
      star.flux = item[11].is_number() ? item[11].get<float>() : 0.0f;
      star.flux_err = item[12].is_number() ? item[12].get<float>() : 0.0f;
      star.flux_qual =
          item[13].is_string() ? item[13].get<std::string>()[0] : ' ';
      star.ids = item[14].get<std::string>();

      // Parse IDs for better name and Catalog ID
      std::stringstream ss(star.ids);
      std::string id_token;
      while (std::getline(ss, id_token, '|')) {
        if (id_token.starts_with("NAME ") && star.name.empty()) {
          star.name = id_token.substr(5);
        } else if (id_token.starts_with("HIP ")) {
          star.catalog = "HIP";
          try {
            star.catalog_id = std::stol(id_token.substr(4));
          } catch (...) {
            star.catalog_id = 0;
          }
        }
      }

      if (star.name.empty()) {
        star.name = item[0].get<std::string>();
      }

      catalog.push_back(star);
    }
  } catch (const nlohmann::json::exception& e) {
    std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
  }

  return catalog;
}

std::shared_ptr<t_calcephbin> CatalogLoader::LoadFromEphemeris(
    const std::filesystem::path& path) {
  t_calcephbin* handle = calceph_open(path.string().c_str());
  if (!handle) {
    std::cerr << "Error: Could not open ephemeris file " << path << std::endl;
    return nullptr;
  }

  return std::shared_ptr<t_calcephbin>(handle, [](t_calcephbin* h) {
    if (h) calceph_close(h);
  });
}

}  // namespace engine
