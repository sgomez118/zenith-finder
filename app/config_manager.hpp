#ifndef APP_CONFIG_MANAGER_HPP_
#define APP_CONFIG_MANAGER_HPP_

#include <filesystem>
#include <optional>
#include <string>

// clang-format off
namespace toml {
  using std::optional;
}
#include <toml++/toml.h>
// clang-format on

#include "engine.hpp"

namespace app {

struct Config {
  engine::Observer observer;
  std::string catalog_path;
  int refresh_rate_ms;
};

class ConfigManager {
 public:
  static Config Load(const std::filesystem::path& path);
  static void Save(const std::filesystem::path& path, const Config& config);
};

}  // namespace app

#endif  // APP_CONFIG_MANAGER_HPP_
