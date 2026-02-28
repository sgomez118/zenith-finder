#define NOMINMAX

#include <objbase.h>

#ifdef _WIN32
#include <conio.h>
#endif

#include <CLI/CLI.hpp>
#include <atomic>
#include <chrono>
#include <csignal>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "app_state.hpp"
#include "catalog_loader.hpp"
#include "config_manager.hpp"
#include "engine.hpp"
#include "location_provider.hpp"
#include "logger.hpp"
#include "ui_style.hpp"
#include "windows_location_provider.hpp"
#include "zenith_ui.hpp"

namespace {
std::atomic<bool>* running_ptr_ = nullptr;

void SignalHandler(int) {
  if (running_ptr_) *running_ptr_ = false;
}

void CalculationWorker(std::shared_ptr<app::AppState> state,
                       std::shared_ptr<app::LocationProvider> provider,
                       std::vector<engine::Star> catalog,
                       std::shared_ptr<app::Logger> logger, bool is_gps,
                       int refresh_ms, std::function<void()> screen_callback) {
  // Initialize COM for Windows Location API
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  engine::AstrometryEngine astrometry_engine;
  astrometry_engine.SetCatalog(catalog);

  while (state->running) {
    auto obs = provider->GetLocation();
    state->gps_active = is_gps;

    {
      std::lock_guard<std::mutex> lock(state->location_mutex);
      state->current_location = obs;
    }

    auto now = std::chrono::system_clock::now();
    auto results = std::make_shared<std::vector<engine::CelestialResult>>(
        astrometry_engine.CalculateZenithProximity(obs, now));
    auto solar_results = std::make_shared<std::vector<engine::SolarBody>>(
        astrometry_engine.CalculateSolarSystem(obs, now));

    if (logger) {
      logger->Log(obs, *results);
    }

    {
      std::lock_guard<std::mutex> lock(state->results_mutex);
      state->latest_results = results;
      state->latest_solar_results = solar_results;
      state->last_calc_time = now;
    }

    // Trigger UI refresh
    if (screen_callback) screen_callback();

    std::this_thread::sleep_for(std::chrono::milliseconds(refresh_ms));
  }

  if (SUCCEEDED(hr)) {
    CoUninitialize();
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::signal(SIGINT, SignalHandler);

  CLI::App app{
      "Zenith Finder - Identify celestial objects at your local zenith"};

  auto config = app::ConfigManager::Load("config.toml");

  engine::Observer obs = config.observer;
  bool use_gps = false;
  bool enable_logging = false;
  std::string catalog_path = config.catalog_path;

  auto lat_opt =
      app.add_option("--lat", obs.latitude, "Observer latitude (degrees)")
          ->check(CLI::Range(-90.0, 90.0));
  auto lon_opt =
      app.add_option("--lon", obs.longitude, "Observer longitude (degrees)")
          ->check(CLI::Range(-180.0, 180.0));
  app.add_option("--alt", obs.altitude, "Observer altitude (meters)")
      ->default_val(0.0);
  app.add_flag("--gps", use_gps, "Use system GPS location service");
  app.add_option("--catalog", catalog_path, "Path to the star catalog CSV file")
      ->check(CLI::ExistingFile);
  app.add_flag("--log", enable_logging,
               "Enable logging to a timestamped CSV file");

  lat_opt->excludes("--gps");
  lon_opt->excludes("--gps");

  CLI11_PARSE(app, argc, argv);

  std::vector<engine::Star> catalog;
  if (catalog_path.ends_with(".json")) {
    catalog = engine::CatalogLoader::LoadStarDataFromJSON(catalog_path);
  } else {
    // catalog = engine::CatalogLoader::LoadStarDataFromCSV(catalog_path);
  }

  if (catalog.empty()) {
    std::cerr << "Error: Could not load catalog from " << catalog_path
              << std::endl;
    return 1;
  }

  // Initialize AppState
  auto state = std::make_shared<app::AppState>();
  running_ptr_ = &state->running;
  state->logging_enabled = enable_logging;

  std::shared_ptr<app::Logger> logger;
  if (enable_logging) {
    logger = std::make_shared<app::Logger>();
    logger->Start();
  }

  std::shared_ptr<app::LocationProvider> provider;
  if (use_gps) {
    provider = std::make_shared<app::WindowsLocationProvider>();
  } else {
    provider = std::make_shared<app::StaticLocationProvider>(obs);
    {
      std::lock_guard<std::mutex> lock(state->location_mutex);
      state->current_location = obs;
    }
  }

  // UI Setup
  app::ZenithUI ui(state);

  // Start worker
  std::thread worker(CalculationWorker, state, provider, std::move(catalog),
                     logger, use_gps, config.refresh_rate_ms,
                     [&ui] { ui.TriggerRefresh(); });

  // Run UI Loop
  ui.Run();

  if (worker.joinable()) worker.join();
  if (logger) logger->Stop();
  app::ConfigManager::Save("config.toml", config);

  return 0;
}
