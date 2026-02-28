#include "zenith_ui.hpp"

#include <chrono>
#include <cmath>
#include <format>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <numbers>
#include <string>

namespace app {

ZenithUI::ZenithUI(std::shared_ptr<AppState> state)
    : state_(std::move(state)),
      screen_(ftxui::ScreenInteractive::Fullscreen()) {}

void ZenithUI::TriggerRefresh() { screen_.Post(ftxui::Event::Custom); }

void ZenithUI::Run() {
  auto renderer = ftxui::Renderer([&] { return Render(); });

  auto event_handler = ftxui::CatchEvent(renderer, [&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q') ||
        event == ftxui::Event::Character('Q')) {
      screen_.Exit();
      state_->running = false;
      return true;
    }
    return false;
  });

  screen_.Loop(event_handler);
}

ftxui::Element ZenithUI::Render() {
  // Fetch Data
  engine::Observer loc;
  {
    std::lock_guard<std::mutex> lock(state_->location_mutex);
    loc = state_->current_location;
  }

  std::shared_ptr<std::vector<engine::CelestialResult>> stars;
  std::shared_ptr<std::vector<engine::SolarBody>> solar;
  std::chrono::system_clock::time_point time;
  {
    std::lock_guard<std::mutex> lock(state_->results_mutex);
    stars = state_->latest_star_results;
    solar = state_->latest_solar_results;
    time = state_->last_calc_time;
  }

  // Time Formatting
  auto time_t = std::chrono::system_clock::to_time_t(time);
  std::tm tm_now;
  // Using gmtime_s for Windows (as in original main.cpp)
  gmtime_s(&tm_now, &time_t);
  std::string time_str =
      std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02} UTC",
                  tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                  tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

  // Sidebar Content
  auto sidebar =
      ftxui::vbox(
          {ftxui::window(
               ftxui::text(" Status "),
               ftxui::vbox({
                   ftxui::text(std::format(
                       "GPS: {}", state_->gps_active ? "Active" : "Manual")) |
                       (state_->gps_active
                            ? ftxui::color(ftxui::Color::Green)
                            : ftxui::color(ftxui::Color::Yellow)),
                   ftxui::text(std::format(
                       "Log: {}", state_->logging_enabled ? "On" : "Off")),
                   ftxui::text("Time: " + time_str),
               })),
           ftxui::window(
               ftxui::text(" Location "),
               ftxui::vbox({
                   ftxui::text(std::format("Lat: {:.4f} N", loc.latitude)),
                   ftxui::text(std::format("Lon: {:.4f} E", loc.longitude)),
                   ftxui::text(std::format("Alt: {:.1f} m", loc.altitude)),
               })),
           ftxui::filler(),
           ftxui::text("Zenith Finder v0.3") | ftxui::dim | ftxui::center}) |
      ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 30);

  // Solar Table
  std::vector<std::vector<ftxui::Element>> solar_rows = {
      {ftxui::text("Body") | ftxui::bold, ftxui::text("Elev") | ftxui::bold,
       ftxui::text("Azimuth") | ftxui::bold,
       ftxui::text("Zenith") | ftxui::bold,
       ftxui::text("Dist (AU)") | ftxui::bold,
       ftxui::text("State") | ftxui::bold}};
  if (solar) {
    for (const auto& body : *solar) {
      if (body.elevation < -12.0) continue;
      auto state_color =
          body.is_rising ? ftxui::Color::Green : ftxui::Color::Red;
      solar_rows.push_back({
          ftxui::text(body.name),
          ftxui::text(std::format("{:.2f}", body.elevation)),
          ftxui::text(std::format("{:.2f}", body.azimuth)),
          ftxui::text(std::format("{:.2f}", body.zenith_dist)),
          ftxui::text(std::format("{:.3f}", body.distance_au)),
          ftxui::text(body.is_rising ? "RISING" : "SETTING") |
              ftxui::color(state_color),
      });
    }
  }
  auto solar_table = ftxui::Table(solar_rows);
  solar_table.SelectAll().Border(ftxui::LIGHT);
  solar_table.SelectRow(0).Decorate(ftxui::bold);
  solar_table.SelectRow(0).SeparatorVertical(ftxui::LIGHT);
  solar_table.SelectRow(0).Border(ftxui::DOUBLE);

  // Star Table (Top 15)
  std::vector<std::vector<ftxui::Element>> star_rows = {
      {ftxui::text("Star") | ftxui::bold, ftxui::text("Elev") | ftxui::bold,
       ftxui::text("Azimuth") | ftxui::bold,
       ftxui::text("Zenith") | ftxui::bold,
       ftxui::text("State") | ftxui::bold}};
  if (stars) {
    int limit = 0;
    for (const auto& star : *stars) {
      if (limit++ > 15) break;
      auto state_color =
          star.is_rising ? ftxui::Color::Green : ftxui::Color::Red;
      star_rows.push_back({
          ftxui::text(star.name),
          ftxui::text(std::format("{:.2f}", star.elevation)),
          ftxui::text(std::format("{:.2f}", star.azimuth)),
          ftxui::text(std::format("{:.2f}", star.zenith_dist)),
          ftxui::text(star.is_rising ? "RISING" : "SETTING") |
              ftxui::color(state_color),
      });
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

        auto color =
            (body.name == "SUN") ? ftxui::Color::Yellow : ftxui::Color::Cyan;
        c.DrawBlockCircle(bx, by, 3, color);
        c.DrawText(bx + 2, by + 2, body.name);
      }
    }
  });

  // Main Layout
  return ftxui::hbox({
             sidebar,
             ftxui::separator(),
             ftxui::vbox({
                 ftxui::hbox({
                     ftxui::window(ftxui::text(" Solar System "),
                                   solar_table.Render()) |
                         ftxui::flex,
                     ftxui::window(ftxui::text(" Zenith Radar "),
                                   radar | ftxui::center) |
                         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60),
                 }),
                 ftxui::window(ftxui::text(" Zenith Stars "),
                               star_table.Render()) |
                     ftxui::flex,
             }) | ftxui::flex,
         }) |
         ftxui::border;
}

}  // namespace app
