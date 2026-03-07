#ifndef ZENITH_FINDER_APP_APP_STATE_HPP_
#define ZENITH_FINDER_APP_APP_STATE_HPP_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "engine.hpp"

namespace app {

struct FilterCriteria {
  std::string name_filter;
  float min_elevation = 0.0f;
  float max_elevation = 90.0f;
  float min_azimuth = 0.0f;
  float max_azimuth = 360.0f;
  bool active = false;
};

struct AppState {
  std::atomic<bool> running{true};
  std::atomic<bool> gps_active{false};
  bool logging_enabled{false};

  std::mutex location_mutex;
  engine::Observer current_location{0.0, 0.0, 0.0};

  std::mutex results_mutex;
  std::shared_ptr<std::vector<engine::CelestialResult>> latest_star_results;
  std::shared_ptr<std::vector<engine::SolarBody>> latest_solar_results;
  std::chrono::system_clock::time_point last_calc_time;

  std::mutex filter_mutex;
  FilterCriteria filter;
  bool show_filter_window = false;
};

}  // namespace app

#endif  // ZENITH_FINDER_APP_APP_STATE_HPP_
