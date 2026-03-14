#include "app_controller.hpp"

#define NOMINMAX
#include <objbase.h>

#include <chrono>
#include <iostream>

#include "catalog_loader.hpp"
#include "windows_location_provider.hpp"

namespace app {

AppController::AppController() : state_(std::make_shared<AppState>()) {}

AppController::~AppController() { Stop(); }

bool AppController::Initialize(const AppConfig& config) {
  config_ = config;
  state_->logging_enabled = config_.enable_logging;

  // 1. Load Star Catalog
  if (config_.catalog_path.ends_with(".json")) {
    catalog_ =
        engine::CatalogLoader::LoadStarDataFromJSON(config_.catalog_path);
  }

  if (catalog_.empty()) {
    std::cerr << "Error: Could not load catalog from " << config_.catalog_path
              << std::endl;
    return false;
  }

  // 2. Load Ephemeris
  if (!config_.ephemeris_path.empty()) {
    ephemeris_ =
        engine::CatalogLoader::LoadFromEphemeris(config_.ephemeris_path);
  }

  // 3. Setup Location Provider
  if (config_.use_gps) {
    location_provider_ = std::make_shared<WindowsLocationProvider>();
  } else {
    location_provider_ =
        std::make_shared<StaticLocationProvider>(config_.manual_location);
    {
      std::lock_guard<std::mutex> lock(state_->location_mutex);
      state_->current_location = config_.manual_location;
    }
  }

  // 4. Setup Logger
  if (config_.enable_logging) {
    logger_ = std::make_shared<Logger>();
  }

  // 5. Setup Engine
  engine_.SetCatalog(catalog_);
  if (ephemeris_) {
    engine_.SetEphemeris(ephemeris_);
  }
  result_buffer_.reserve(catalog_.size(), 15);

  return true;
}

void AppController::Start() {
  if (state_->running && !worker_thread_) {
    if (logger_) {
      logger_->Start();
    }
    worker_thread_ =
        std::make_unique<std::thread>(&AppController::RunWorker, this);
  }
}

void AppController::Stop() {
  state_->running = false;
  if (worker_thread_ && worker_thread_->joinable()) {
    worker_thread_->join();
    worker_thread_.reset();
  }
  if (logger_) {
    logger_->Stop();
  }
}

void AppController::RunWorker() {
  // Initialize COM for this thread (Windows specific)
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  while (state_->running) {
    auto obs = location_provider_->GetLocation();
    state_->gps_active = config_.use_gps;

    {
      std::lock_guard<std::mutex> lock(state_->location_mutex);
      state_->current_location = obs;
    }

    auto now = std::chrono::system_clock::now();

    engine::FilterCriteria engine_filter;
    engine::SortCriteria star_sort;
    engine::SortCriteria solar_sort;
    {
      std::lock_guard<std::mutex> lock(state_->filter_mutex);
      engine_filter = state_->filter;
    }
    {
      std::lock_guard<std::mutex> lock(state_->sort_mutex);
      star_sort = state_->star_sort;
      solar_sort = state_->solar_sort;
    }

    // Use persistent buffer to minimize heap churn
    engine_.CalculateZenithProximity(result_buffer_, obs, engine_filter,
                                     star_sort, now);
    engine_.CalculateSolarSystem(result_buffer_, obs, engine_filter, solar_sort,
                                 now);

    // Snapshot results for the UI thread
    auto star_results = std::make_shared<std::vector<engine::CelestialResult>>(
        result_buffer_.star_results);
    auto solar_results = std::make_shared<std::vector<engine::SolarBody>>(
        result_buffer_.solar_results);

    if (logger_) {
      logger_->Log(obs, *star_results);
    }

    {
      std::lock_guard<std::mutex> lock(state_->results_mutex);
      state_->latest_star_results = star_results;
      state_->latest_solar_results = solar_results;
      state_->last_calc_time = now;
    }

    // Trigger UI refresh
    if (refresh_callback_) {
      refresh_callback_();
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.refresh_rate_ms));
  }

  if (SUCCEEDED(hr)) {
    CoUninitialize();
  }
}

}  // namespace app
