#include "catalog_loader.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace engine {

namespace {

// Finds the value associated with a key from the list of pipe separated
// identifiers.
std::string GetValueFromIds(const std::string& ids, const std::string& key) {
  for (const auto& id_range : std::views::split(std::string_view(ids), '|')) {
    // Convert range to string to handle potential non-contiguous iterators in
    // C++20 split
    std::string entry(id_range.begin(), id_range.end());
    std::string_view entry_view(entry);

    // Trim leading and trailing spaces from the entry
    if (auto start = entry_view.find_first_not_of(' ');
        start != std::string_view::npos) {
      entry_view = entry_view.substr(start);
      if (auto end = entry_view.find_last_not_of(' ');
          end != std::string_view::npos) {
        entry_view = entry_view.substr(0, end + 1);
      }
    } else {
      continue;
    }

    // Match the key at the start of the entry
    if (entry_view.starts_with(key)) {
      std::string_view val = entry_view.substr(key.size());
      // Key must be the entire entry or followed by a space
      if (val.empty()) {
        return "";
      }
      if (val.front() == ' ') {
        // Trim leading spaces from the value and return it
        if (auto vstart = val.find_first_not_of(' ');
            vstart != std::string_view::npos) {
          return std::string(val.substr(vstart));
        }
        return "";
      }
    }
  }

  return "";
}

// Finds the name from the identifiers or returns the default name.
std::string GetStarNameFromIds(const std::string& ids,
                               const std::string& default_name) {
  std::string name = GetValueFromIds(ids, "NAME");
  if (!name.empty()) {
    return name;
  }

  return default_name;
}

// Finds the catalog name (HIP, FK5, GC) and ID from the identifiers.
std::tuple<std::string, long> GetStarNameAndCatalogIdFromIds(
    const std::string& ids) {
  std::string catalog_name = "HIP";
  long catalog_id = 0;

  std::string id_string = GetValueFromIds(ids, catalog_name);
  if (!id_string.empty()) {
    try {
      catalog_id = std::stol(id_string);
    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
    }
  } else {
    catalog_name = "FK5";
    id_string = GetValueFromIds(ids, catalog_name);
    if (!id_string.empty()) {
      try {
        catalog_id = std::stol(id_string);
      } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
      }
    } else {
      catalog_name = "GC";
      id_string = GetValueFromIds(ids, catalog_name);
      if (!id_string.empty()) {
        try {
          catalog_id = std::stol(id_string);
        } catch (const std::exception& e) {
          std::cerr << e.what() << '\n';
        }
      } else {
        catalog_name = "Unknown";
      }
    }
  }
  return {catalog_name, catalog_id};
}

}  // namespace

std::vector<Star> CatalogLoader::LoadStarDataFromJSON(
    const std::filesystem::path& path) {
  std::vector<Star> stars;
  std::ifstream file(path);

  if (!file.is_open()) {
    return stars;
  }

  try {
    nlohmann::json json;
    file >> json;

    if (json.contains("data") && json["data"].is_array()) {
      for (const auto& row : json["data"]) {
        if (row.is_array() && row.size() >= 15) {
          std::string ids = row[14].is_null() ? "" : row[14].get<std::string>();

          std::string star_name =
              row[0].is_null() ? "" : row[0].get<std::string>();
          star_name = GetStarNameFromIds(ids, star_name);

          auto [catalog_name, catalog_id] = GetStarNameAndCatalogIdFromIds(ids);

          Star star;
          star.name = star_name;
          star.catalog = catalog_name;
          star.catalog_id = catalog_id;
          star.ra = row[1].is_null() ? 0.0 : row[1].get<double>();
          star.dec = row[2].is_null() ? 0.0 : row[2].get<double>();
          star.coo_qual = row[3].is_null() ? ' ' : row[3].get<std::string>()[0];
          star.pmra = row[4].is_null() ? 0.0 : row[4].get<double>();
          star.pmdec = row[5].is_null() ? 0.0 : row[5].get<double>();
          star.pm_qual = row[6].is_null() ? ' ' : row[6].get<std::string>()[0];
          star.parallax = row[7].is_null() ? 0.0 : row[7].get<double>();
          star.plx_qual = row[8].is_null() ? ' ' : row[8].get<std::string>()[0];
          star.radial_velocity = row[9].is_null() ? 0.0 : row[9].get<double>();
          star.rvz_qual =
              row[10].is_null() ? ' ' : row[10].get<std::string>()[0];
          star.flux = row[11].is_null() ? 0.0f : row[11].get<float>();
          star.flux_err = row[12].is_null() ? 0.0f : row[12].get<float>();
          star.flux_qual =
              row[13].is_null() ? ' ' : row[13].get<std::string>()[0];
          star.ids = ids;
          stars.push_back(star);
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error parsing JSON catalog: " << e.what() << '\n';
  }

  return stars;
}

std::shared_ptr<t_calcephbin> CatalogLoader::LoadFromEphemeris(
    const std::filesystem::path& path) {
  std::shared_ptr<t_calcephbin> ephemeris;
  std::ifstream file(path);

  if (!file.is_open()) {
    return ephemeris;
  }

  auto ephem = calceph_open(path.string().c_str());
  if (ephem) {
    ephemeris = std::shared_ptr<t_calcephbin>(ephem, calceph_close);
  }

  return ephemeris;
}

}  // namespace engine
