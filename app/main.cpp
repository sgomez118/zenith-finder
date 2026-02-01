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
#include <numbers>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/dom/canvas.hpp>
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

// Google Style Guide: Do not use using-directives (e.g. using namespace foo).
// Fully qualified names are used below.

namespace {
std::atomic<bool> g_running{true};
std::mutex g_results_mutex;
std::shared_ptr<std::vector<engine::CelestialResult>> g_latest_results;
std::shared_ptr<std::vector<engine::SolarBody>> g_latest_solar_results;
std::chrono::system_clock::time_point g_last_calc_time;

std::mutex g_location_mutex;
engine::Observer g_current_location{0.0, 0.0, 0.0};
std::atomic<bool> g_gps_active{false};

void SignalHandler(int) { g_running = false; }

void CalculationWorker(std::shared_ptr<app::LocationProvider> provider,
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
  auto screen = ftxui::ScreenInteractive::Fullscreen();

  // UI Components
  auto renderer = ftxui::Renderer([&] {
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
    std::string time_str =
        std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02} UTC",
                    tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                    tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

    // Sidebar Content
    auto sidebar =
        ftxui::vbox({ftxui::window(ftxui::text(" Status "),
                     ftxui::vbox({
                         ftxui::text(std::format("GPS: {}",
                                          g_gps_active ? "Active" : "Manual")) |
                             (g_gps_active ? ftxui::color(ftxui::Color::Green)
                                           : ftxui::color(ftxui::Color::Yellow)),
                         ftxui::text(std::format("Log: {}",
                                          enable_logging ? "On" : "Off")),
                         ftxui::text("Time: " + time_str),
                     })),
              ftxui::window(ftxui::text(" Location "),
                     ftxui::vbox({
                         ftxui::text(std::format("Lat: {:.4f} N", loc.latitude)),
                         ftxui::text(std::format("Lon: {:.4f} E", loc.longitude)),
                         ftxui::text(std::format("Alt: {:.1f} m", loc.altitude)),
                     })),
              ftxui::filler(), ftxui::text("Zenith Finder v0.3") | ftxui::dim | ftxui::center}) |
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 30);

    // Solar Table
    std::vector<std::vector<ftxui::Element>> solar_rows = {
        {ftxui::text("Body") | ftxui::bold, ftxui::text("Elev") | ftxui::bold, ftxui::text("Azimuth") | ftxui::bold,
         ftxui::text("Zenith") | ftxui::bold, ftxui::text("Dist (AU)") | ftxui::bold,
         ftxui::text("State") | ftxui::bold}};
    if (solar) {
      for (const auto& body : *solar) {
        if (body.elevation < -12.0) continue;
        auto state_color = body.is_rising ? ftxui::Color::Green : ftxui::Color::Red;
        solar_rows.push_back(
            {ftxui::text(body.name), ftxui::text(std::format("{:.2f}", body.elevation)),
             ftxui::text(std::format("{:.2f}", body.azimuth)),
             ftxui::text(std::format("{:.2f}", body.zenith_dist)),
             ftxui::text(std::format("{:.3f}", body.distance_au)),
             ftxui::text(body.is_rising ? "RISING" : "SETTING") | ftxui::color(state_color)});
      }
    }
    auto solar_table = ftxui::Table(solar_rows);
    solar_table.SelectAll().Border(ftxui::LIGHT);
    solar_table.SelectRow(0).Decorate(ftxui::bold);
    solar_table.SelectRow(0).SeparatorVertical(ftxui::LIGHT);
    solar_table.SelectRow(0).Border(ftxui::DOUBLE);

    // Star Table (Top 15)
    std::vector<std::vector<ftxui::Element>> star_rows = {
        {ftxui::text("Star") | ftxui::bold, ftxui::text("Elev") | ftxui::bold, ftxui::text("Azimuth") | ftxui::bold,
         ftxui::text("Zenith") | ftxui::bold, ftxui::text("State") | ftxui::bold}};
    if (stars) {
      int limit = 0;
      for (const auto& star : *stars) {
        if (limit++ > 15) break;
        auto state_color = star.is_rising ? ftxui::Color::Green : ftxui::Color::Red;
        star_rows.push_back(
            {ftxui::text(star.name), ftxui::text(std::format("{:.2f}", star.elevation)),
             ftxui::text(std::format("{:.2f}", star.azimuth)),
             ftxui::text(std::format("{:.2f}", star.zenith_dist)),
             ftxui::text(star.is_rising ? "RISING" : "SETTING") | ftxui::color(state_color)});
      }
    }
    auto star_table = ftxui::Table(star_rows);
    star_table.SelectAll().Border(ftxui::LIGHT);
    star_table.SelectRow(0).Decorate(ftxui::bold);
    star_table.SelectRow(0).SeparatorVertical(ftxui::LIGHT);
    star_table.SelectRow(0).Border(ftxui::DOUBLE);

    // Zenith Radar Canvas
    auto radar = ftxui::canvas(100, 100, [&](ftxui::Canvas& c) {
      // Center and Radius
      int cx = 50;
      int cy = 50;
      int r = 45;

      // Draw Horizon
      for (int i = 0; i < 360; i += 5) {
        double angle = i * std::numbers::pi / 180.0;
        int x1 = cx + static_cast<int>(r * std::cos(angle));
        int y1 = cy + static_cast<int>(r * std::sin(angle));
        int x2 = cx + static_cast<int>((r + 2) * std::cos(angle));
        int y2 = cy + static_cast<int>((r + 2) * std::sin(angle));
        c.DrawBlockLine(x1, y1, x2, y2, ftxui::Color::GrayDark);
      }

      // Draw Cardinal points
      c.DrawText(cx, cy - r - 5, "N");
      c.DrawText(cx + r + 5, cy, "E");
      c.DrawText(cx, cy + r + 5, "S");
      c.DrawText(cx - r - 8, cy, "W");

      // Draw Stars
      if (stars) {
        for (const auto& star : *stars) {
          // Polar to Cartesian
          // r_star = r * (90 - el) / 90
          double r_s = r * (star.zenith_dist / 90.0);
          double az_rad = (star.azimuth - 90.0) * std::numbers::pi / 180.0;
          int sx = cx + static_cast<int>(r_s * std::cos(az_rad));
          int sy = cy + static_cast<int>(r_s * std::sin(az_rad));

          if (star.zenith_dist < 2.0) {
             c.DrawBlockCircle(sx, sy, 2, ftxui::Color::Yellow);
          } else {
             c.DrawBlock(sx, sy, true, ftxui::Color::White);
          }
        }
      }

      // Draw Solar bodies
      if (solar) {
        for (const auto& body : *solar) {
          if (body.elevation < 0) continue;
          double r_b = r * (body.zenith_dist / 90.0);
          double az_rad = (body.azimuth - 90.0) * std::numbers::pi / 180.0;
          int bx = cx + static_cast<int>(r_b * std::cos(az_rad));
          int by = cy + static_cast<int>(r_b * std::sin(az_rad));
          
          auto color = (body.name == "Sun") ? ftxui::Color::Yellow : ftxui::Color::Cyan;
          c.DrawBlockCircle(bx, by, 3, color);
          c.DrawText(bx + 2, by + 2, body.name);
        }
      }
    });

    // Main Layout
    return ftxui::hbox({sidebar, ftxui::separator(),
                 ftxui::vbox({
                     ftxui::hbox({
                         ftxui::window(ftxui::text(" Solar System "), solar_table.Render()) | ftxui::flex,
                         ftxui::window(ftxui::text(" Zenith Radar "), radar | ftxui::center) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60)
                     }),
                     ftxui::window(ftxui::text(" Zenith Stars "), star_table.Render()) | ftxui::flex
                 }) | ftxui::flex}) |
           ftxui::border;
  });

  auto event_handler = ftxui::CatchEvent(renderer, [&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) {
      screen.Exit();
      g_running = false;
      return true;
    }
    return false;
  });

  // Start worker
  std::thread worker(CalculationWorker, provider, std::move(catalog), logger,
                     use_gps, config.refresh_rate_ms, [&screen] {
                       screen.Post(ftxui::Event::Custom);  // Trigger refresh
                     });

  screen.Loop(event_handler);

  if (worker.joinable()) worker.join();
  if (logger) logger->Stop();
  app::ConfigManager::Save("config.toml", config);

  return 0;
}
