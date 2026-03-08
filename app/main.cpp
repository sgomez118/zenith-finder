#define NOMINMAX

#include <objbase.h>

#ifdef _WIN32
#include <conio.h>
#endif

#include <CLI/CLI.hpp>
#include <csignal>
#include <iostream>
#include <memory>

#include "app_controller.hpp"
#include "config_manager.hpp"
#include "ui/zenith_ui.hpp"

namespace {
std::shared_ptr<app::AppController> global_controller;

void SignalHandler(int) {
  if (global_controller) {
    global_controller->Stop();
  }
}
}  // namespace

int main(int argc, char** argv) {
  // Initialize COM for the main thread (needed for Location API)
  HRESULT hr_com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

  std::signal(SIGINT, SignalHandler);

  CLI::App app{
      "Zenith Finder - Identify celestial objects at your local zenith"};

  auto config_file = app::ConfigManager::Load("config.toml");

  app::AppConfig app_config;
  app_config.manual_location = config_file.observer;
  app_config.catalog_path = config_file.catalog_path;
  app_config.ephemeris_path = config_file.ephemeris_path;
  app_config.refresh_rate_ms = config_file.refresh_rate_ms;

  app.add_option("--lat", app_config.manual_location.latitude,
                 "Observer latitude (degrees)")
      ->check(CLI::Range(-90.0, 90.0));
  app.add_option("--lon", app_config.manual_location.longitude,
                 "Observer longitude (degrees)")
      ->check(CLI::Range(-180.0, 180.0));
  app.add_option("--alt", app_config.manual_location.altitude,
                 "Observer altitude (meters)")
      ->default_val(0.0);
  app.add_flag("--gps", app_config.use_gps, "Use system GPS location service");
  app.add_option("--catalog", app_config.catalog_path,
                 "Path to the star catalog CSV file")
      ->check(CLI::ExistingFile);
  app.add_flag("--log", app_config.enable_logging,
               "Enable logging to a timestamped CSV file");

  CLI11_PARSE(app, argc, argv);

  auto controller = std::make_shared<app::AppController>();
  global_controller = controller;

  if (!controller->Initialize(app_config)) {
    if (SUCCEEDED(hr_com)) CoUninitialize();
    return 1;
  }

  // UI Setup
  app::ZenithUI ui(controller->GetState());

  // Wire up the refresh callback
  controller->SetRefreshCallback([&ui] { ui.TriggerRefresh(); });

  // Start the background tasks
  controller->Start();

  // Run UI Loop (Blocks until exit)
  ui.Run();

  // Cleanup
  controller->Stop();

  // Save config back (update with any changes)
  config_file.observer = app_config.manual_location;
  config_file.catalog_path = app_config.catalog_path;
  app::ConfigManager::Save("config.toml", config_file);

  if (SUCCEEDED(hr_com)) {
    CoUninitialize();
  }

  return 0;
}
