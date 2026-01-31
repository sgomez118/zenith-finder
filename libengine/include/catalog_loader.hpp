#ifndef LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
#define LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_

#include <filesystem>
#include <string>
#include <vector>

#include "engine.hpp"

namespace engine {

class CatalogLoader {
 public:
  static std::vector<Star> LoadFromCSV(const std::filesystem::path& path);
};

}  // namespace engine

#endif  // LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
