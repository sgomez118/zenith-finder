#ifndef APP_APP_STATE_HPP_
#define APP_APP_STATE_HPP_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

#include "engine.hpp"

namespace app {

struct AppState {
  std::atomic<bool> running{true};
  std::atomic<bool> gps_active{false};
  bool logging_enabled{false};

  std::mutex location_mutex;
  engine::Observer current_location{0.0, 0.0, 0.0};

  std::mutex results_mutex;
  std::shared_ptr<std::vector<engine::CelestialResult>> latest_results;
  std::shared_ptr<std::vector<engine::SolarBody>> latest_solar_results;
  std::chrono::system_clock::time_point last_calc_time;
};

}  // namespace app

#endif  // APP_APP_STATE_HPP_
