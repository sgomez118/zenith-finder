#include <objbase.h>  // For CoInitializeEx

#ifdef _WIN32
#include <conio.h>
#endif

#include <CLI/CLI.hpp>
#include <atomic>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "catalog_loader.hpp"
#include "engine.hpp"
#include "location_provider.hpp"
#include "logger.hpp"
#include "ui_style.hpp"
#include "windows_location_provider.hpp"

namespace {
std::atomic<bool> g_running{true};
std::mutex g_results_mutex;
std::shared_ptr<std::vector<engine::CelestialResult>> g_latest_results;
std::chrono::system_clock::time_point g_last_calc_time;

// Also store the latest observer location for display
std::mutex g_location_mutex;
engine::Observer g_current_location{0.0, 0.0, 0.0};
std::atomic<bool> g_gps_active{false};

void signal_handler(int) { g_running = false; }

void calculation_worker(std::shared_ptr<app::LocationProvider> provider,
                        std::vector<engine::Star> catalog,
                        std::shared_ptr<app::Logger> logger, bool is_gps) {
  // Initialize COM for Windows Location API
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  while (g_running) {
    auto obs = provider->GetLocation();
    g_gps_active = is_gps;

    {
      std::lock_guard<std::mutex> lock(g_location_mutex);
      g_current_location = obs;
    }

    auto now = std::chrono::system_clock::now();
    auto results = std::make_shared<std::vector<engine::CelestialResult>>(
        engine::AstrometryEngine::CalculateZenithProximity(obs, catalog, now));

    if (logger) {
      logger->Log(obs, *results);
    }

    {
      std::lock_guard<std::mutex> lock(g_results_mutex);
      g_latest_results = results;
      g_last_calc_time = now;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  if (SUCCEEDED(hr)) {
    CoUninitialize();
  }
}
}  // namespace

int main(int argc, char** argv) {
  std::signal(SIGINT, signal_handler);

  CLI::App app{
      "Zenith Finder - Identify celestial objects at your local zenith"};

  engine::Observer obs{0.0, 0.0, 0.0};
  bool use_gps = false;
  bool enable_logging = false;
  std::string catalog_path = "stars.csv";

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

  // Validation: Either GPS or (Lat and Lon) must be provided
  lat_opt->excludes("--gps");
  lon_opt->excludes("--gps");

  CLI11_PARSE(app, argc, argv);

  // Manual validation fallback
  if (!use_gps && (lat_opt->count() == 0 || lon_opt->count() == 0)) {
    std::cerr << "Error: Either --gps or both --lat and --lon are required."
              << std::endl;
    return 1;
  }

  auto catalog = engine::CatalogLoader::LoadFromCSV(catalog_path);
  if (catalog.empty()) {
    std::cerr << "Error: Could not load catalog from " << catalog_path
              << " or file is empty." << std::endl;
    return 1;
  }

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
      std::lock_guard<std::mutex> lock(g_location_mutex);
      g_current_location = obs;
    }
  }

  // Start worker thread
  std::thread worker(calculation_worker, provider, std::move(catalog), logger,
                     use_gps);

  // Clear screen once at start
  std::cout << "\033[2J";

  auto next_tick = std::chrono::steady_clock::now();

  while (g_running) {
    // Handle input
#ifdef _WIN32
    if (_kbhit()) {
      int ch = _getch();
      if (ch == 'q' || ch == 'Q') {
        g_running = false;
      }
    }
#endif

    auto now = std::chrono::system_clock::now();

    // Home cursor
    std::cout << "\033[H";

    // Header info
    std::cout << app::ui::BG_BLUE << app::ui::WHITE << app::ui::BOLD
              << "============================================================="
                 "========="
              << app::ui::RESET << "\n";

    std::cout << app::ui::BG_BLUE << app::ui::WHITE << app::ui::BOLD
              << std::format(
                     " ZENITH FINDER v0.2 | GPS: {:<9} | Log: {:<3}            "
                     "         ",
                     (g_gps_active ? "CONNECTED" : "MANUAL"),
                     (enable_logging ? "ON" : "OFF"))
              << app::ui::RESET << "\n";

    std::cout << app::ui::BG_BLUE << app::ui::WHITE << app::ui::BOLD
              << "============================================================="
                 "========="
              << app::ui::RESET << "\n";

    engine::Observer current_obs;
    {
      std::lock_guard<std::mutex> lock(g_location_mutex);
      current_obs = g_current_location;
    }
    std::cout << std::format(
        "{}Location:{} {:.4f}N, {:.4f}E, {:.1f}m             \n", app::ui::CYAN,
        app::ui::RESET, current_obs.latitude, current_obs.longitude,
        current_obs.altitude);

    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
#ifdef _WIN32
    gmtime_s(&now_tm, &now_time_t);
#else
    gmtime_r(&now_time_t, &now_tm);
#endif
    std::cout << std::format(
        "{}Time (UTC):{} {:04}-{:02}-{:02} {:02}:{:02}:{:02}\n", app::ui::CYAN,
        app::ui::RESET, now_tm.tm_year + 1900, now_tm.tm_mon + 1,
        now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);

    std::cout << app::ui::DIM
              << "-------------------------------------------------------------"
                 "---------"
              << app::ui::RESET << "\n";
    std::cout << std::format(
        "{}{:<20} | {:>10} | {:>10} | {:>10} | {:<10} {}\n", app::ui::BOLD,
        "Star Name", "Elevation", "Azimuth", "Zenith Dist", "Status",
        app::ui::RESET);
    std::cout << app::ui::DIM
              << "-------------------------------------------------------------"
                 "---------"
              << app::ui::RESET << "\n";

    std::shared_ptr<std::vector<engine::CelestialResult>> current_results;
    {
      std::lock_guard<std::mutex> lock(g_results_mutex);
      current_results = g_latest_results;
    }

    if (current_results) {
      for (const auto& res : *current_results) {
        std::string status_color = res.is_rising ? std::string(app::ui::GREEN)
                                                 : std::string(app::ui::RED);
        std::string status_icon = res.is_rising
                                      ? std::string(app::ui::ICON_RISING)
                                      : std::string(app::ui::ICON_SETTING);
        std::string status_text = res.is_rising ? "RISING" : "SETTING";

        std::cout << std::format(
            "{:<20} | {:>10.2f} | {:>10.2f} | {:>10.2f} | {}{:<8} {}{}\n",
            res.name, res.elevation, res.azimuth, res.zenith_dist, status_color,
            status_text, status_icon, app::ui::RESET);
      }

      if (current_results->empty()) {
        std::cout << app::ui::YELLOW
                  << "No stars from the catalog are currently above the "
                     "horizon.            "
                  << app::ui::RESET << "\n";
      }
    } else {
      std::cout << app::ui::YELLOW
                << "Calculating...                                             "
                   "              "
                << app::ui::RESET << "\n";
    }

    // Clear remaining lines if any
    std::cout << "\033[J";

    std::cout << "\n"
              << app::ui::CYAN << "Controls: " << app::ui::RESET
              << "[Q] Quit\n";

    next_tick += std::chrono::seconds(1);
    std::this_thread::sleep_until(next_tick);
  }

  if (worker.joinable()) {
    worker.join();
  }

  if (logger) {
    logger->Stop();
  }

  std::cout << "\nExiting...\n";

  return 0;
}
