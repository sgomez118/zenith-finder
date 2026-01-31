#ifndef LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
#define LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_

#include "engine.hpp"
#include <vector>
#include <string>
#include <filesystem>

namespace engine {

class CatalogLoader {
public:
    static std::vector<Star> LoadFromCSV(const std::filesystem::path& path);
};

} // namespace engine

#endif // LIBENGINE_INCLUDE_CATALOG_LOADER_HPP_
