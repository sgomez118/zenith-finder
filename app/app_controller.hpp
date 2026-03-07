#ifndef ZENITH_FINDER_APP_APP_CONTROLLER_HPP_
#define ZENITH_FINDER_APP_APP_CONTROLLER_HPP_

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "app_state.hpp"
#include "engine.hpp"
#include "location_provider.hpp"
#include "logger.hpp"

namespace app {

struct AppConfig {
  engine::Observer manual_location;
  bool use_gps = false;
  bool enable_logging = false;
  std::string catalog_path;
  std::string ephemeris_path;
  int refresh_rate_ms = 1000;
};

class AppController {
 public:
  AppController();
  ~AppController();

  // Prevent copying
  AppController(const AppController&) = delete;
  AppController& operator=(const AppController&) = delete;

  bool Initialize(const AppConfig& config);
  void Start();
  void Stop();

  std::shared_ptr<AppState> GetState() const { return state_; }

  // Callback to trigger UI refresh (or other notifications)
  void SetRefreshCallback(std::function<void()> callback) {
    refresh_callback_ = std::move(callback);
  }

 private:
  void RunWorker();

  std::shared_ptr<AppState> state_;
  AppConfig config_;

  std::shared_ptr<LocationProvider> location_provider_;
  std::shared_ptr<Logger> logger_;
  std::unique_ptr<std::thread> worker_thread_;
  
  std::function<void()> refresh_callback_;
  
  // Cache for engine data
  std::vector<engine::Star> catalog_;
  std::shared_ptr<t_calcephbin> ephemeris_;
};

}  // namespace app

#endif  // ZENITH_FINDER_APP_APP_CONTROLLER_HPP_
