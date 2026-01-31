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
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "catalog_loader.hpp"
#include "config_manager.hpp"
#include "engine.hpp"
#include "location_provider.hpp"
#include "logger.hpp"
#include "ui_style.hpp"
#include "windows_location_provider.hpp"

using namespace ftxui;

namespace {
std::atomic<bool> g_running{true};
std::mutex g_results_mutex;
std::shared_ptr<std::vector<engine::CelestialResult>> g_latest_results;
std::shared_ptr<std::vector<engine::SolarBody>> g_latest_solar_results;
std::chrono::system_clock::time_point g_last_calc_time;

std::mutex g_location_mutex;
engine::Observer g_current_location{0.0, 0.0, 0.0};
std::atomic<bool> g_gps_active{false};

void signal_handler(int) { g_running = false; }

void calculation_worker(std::shared_ptr<app::LocationProvider> provider,
                        std::vector<engine::Star> catalog,
                        std::shared_ptr<app::Logger> logger, bool is_gps,
                        int refresh_ms, std::function<void()> screen_callback) {
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
    auto solar_results = std::make_shared<std::vector<engine::SolarBody>>(
        engine::AstrometryEngine::CalculateSolarSystem(obs, now));

    if (logger) {
      logger->Log(obs, *results);
    }

    {
      std::lock_guard<std::mutex> lock(g_results_mutex);
      g_latest_results = results;
      g_latest_solar_results = solar_results;
      g_last_calc_time = now;
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
  std::signal(SIGINT, signal_handler);

  CLI::App app{"Zenith Finder - Identify celestial objects at your local zenith"};

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
  app.add_flag("--log", enable_logging, "Enable logging to a timestamped CSV file");

  lat_opt->excludes("--gps");
  lon_opt->excludes("--gps");

  CLI11_PARSE(app, argc, argv);

  auto catalog = engine::CatalogLoader::LoadFromCSV(catalog_path);
  if (catalog.empty()) {
    std::cerr << "Error: Could not load catalog from " << catalog_path
              << std::endl;
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

  // FTXUI Setup
  auto screen = ScreenInteractive::Fullscreen();

  // UI Components
  auto renderer = Renderer([&] {
    // Fetch Data
    engine::Observer loc;
    {
      std::lock_guard<std::mutex> lock(g_location_mutex);
      loc = g_current_location;
    }

    std::shared_ptr<std::vector<engine::CelestialResult>> stars;
    std::shared_ptr<std::vector<engine::SolarBody>> solar;
    std::chrono::system_clock::time_point time;
    {
      std::lock_guard<std::mutex> lock(g_results_mutex);
      stars = g_latest_results;
      solar = g_latest_solar_results;
      time = g_last_calc_time;
    }

    // Time Formatting
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm_now;
    gmtime_s(&tm_now, &time_t);
    std::string time_str = std::format(
        "{:04}-{:02}-{:02} {:02}:{:02}:{:02} UTC", tm_now.tm_year + 1900,
        tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min,
        tm_now.tm_sec);

    // Sidebar Content
    auto sidebar = vbox({window(text(" Status "),
                                vbox({
                                    text(std::format("GPS: {}", g_gps_active
                                                                    ? "Active"
                                                                    : "Manual")) |
                                        (g_gps_active ? color(Color::Green)
                                                      : color(Color::Yellow)),
                                    text(std::format("Log: {}", enable_logging
                                                                    ? "On"
                                                                    : "Off")),
                                    text("Time: " + time_str),
                                })),
                         window(text(" Location "),
                                vbox({
                                    text(std::format("Lat: {:.4f} N",
                                                     loc.latitude)),
                                    text(std::format("Lon: {:.4f} E",
                                                     loc.longitude)),
                                    text(std::format("Alt: {:.1f} m",
                                                     loc.altitude)),
                                })),
                         filler(), text("Zenith Finder v0.3") | dim | center}) |
                   size(WIDTH, EQUAL, 30);

    // Solar Table
    std::vector<std::vector<Element>> solar_rows = {
        {text("Body") | bold, text("Elev") | bold, text("Azimuth") | bold,
         text("Zenith") | bold, text("Dist (AU)") | bold, text("State") | bold}};
    if (solar) {
      for (const auto& body : *solar) {
        if (body.elevation < -12.0) continue;
        auto state_color = body.is_rising ? Color::Green : Color::Red;
        solar_rows.push_back({text(body.name),
                              text(std::format("{:.2f}", body.elevation)),
                              text(std::format("{:.2f}", body.azimuth)),
                              text(std::format("{:.2f}", body.zenith_dist)),
                              text(std::format("{:.3f}", body.distance_au)),
                              text(body.is_rising ? "RISING" : "SETTING") |
                                  color(state_color)});
      }
    }
    auto solar_table = Table(solar_rows);
    solar_table.SelectAll().Border(LIGHT);
    solar_table.SelectRow(0).Decorate(bold);
    solar_table.SelectRow(0).SeparatorVertical(LIGHT);
    solar_table.SelectRow(0).Border(ftxui::DOUBLE);

    // Star Table (Top 15)
    std::vector<std::vector<Element>> star_rows = {
        {text("Star") | bold, text("Elev") | bold, text("Azimuth") | bold,
         text("Zenith") | bold, text("State") | bold}};
    if (stars) {
      int limit = 0;
      for (const auto& star : *stars) {
        if (limit++ > 15) break;
        auto state_color = star.is_rising ? Color::Green : Color::Red;
        star_rows.push_back({text(star.name),
                             text(std::format("{:.2f}", star.elevation)),
                             text(std::format("{:.2f}", star.azimuth)),
                             text(std::format("{:.2f}", star.zenith_dist)),
                             text(star.is_rising ? "RISING" : "SETTING") |
                                 color(state_color)});
      }
    }
    auto star_table = Table(star_rows);
    star_table.SelectAll().Border(LIGHT);
    star_table.SelectRow(0).Decorate(bold);
    star_table.SelectRow(0).SeparatorVertical(LIGHT);
    star_table.SelectRow(0).Border(ftxui::DOUBLE);

    // Main Layout
    return hbox({sidebar, separator(),
                 vbox({window(text(" Solar System "), solar_table.Render()),
                       window(text(" Zenith Stars "), star_table.Render()) |
                           flex}) |
                     flex}) |
           border;
  });

  auto event_handler = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Character('Q')) {
      screen.Exit();
      g_running = false;
      return true;
    }
    return false;
  });

  // Start worker
  std::thread worker(calculation_worker, provider, std::move(catalog), logger,
                     use_gps, config.refresh_rate_ms, [&screen] {
                       screen.Post(Event::Custom);  // Trigger refresh
                     });

  screen.Loop(event_handler);

  if (worker.joinable()) worker.join();
  if (logger) logger->Stop();
  app::ConfigManager::Save("config.toml", config);

  return 0;
}
