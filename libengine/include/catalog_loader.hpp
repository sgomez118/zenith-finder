#ifndef ZENITH_FINDER_LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
#define ZENITH_FINDER_LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <calceph.h>
}

#include "engine.hpp"

namespace engine {

// CatalogLoader provides static methods to load astronomical data from
// various file formats, including star catalogs and planetary ephemeris.
class CatalogLoader {
 public:
  // Loads star data from a CSV formatted file.
  static std::vector<Star> LoadStarDataFromCSV(
      const std::filesystem::path& path);

  // Loads star data from a JSON formatted file.
  static std::vector<Star> LoadStarDataFromJSON(
      const std::filesystem::path& path);

  // Loads planetary ephemeris data from a file (e.g., JPL DE405) using CALCEPH.
  // Returns a shared pointer that automatically handles resource cleanup.
  static std::shared_ptr<t_calcephbin> LoadFromEphemeris(
      const std::filesystem::path& path);
};

}  // namespace engine

#endif  // ZENITH_FINDER_LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
