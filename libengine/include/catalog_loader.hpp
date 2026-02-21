#ifndef LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
#define LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_

#include <filesystem>
#include <string>
#include <vector>

extern "C" {
#include <calceph.h>
}

#include "engine.hpp"

namespace engine {

class CatalogLoader {
 public:
  static std::vector<Star> LoadStarDataFromCSV(
      const std::filesystem::path& path);
  static std::vector<Star> LoadStarDataFromJSON(
      const std::filesystem::path& path);
  static std::shared_ptr<t_calcephbin> LoadFromEphemeris(
      const std::filesystem::path& path);
};

}  // namespace engine

#endif  // LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
