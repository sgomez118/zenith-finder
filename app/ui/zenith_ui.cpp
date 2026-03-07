#include "zenith_ui.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <numbers>
#include <string>

#include "ui_style.hpp"

namespace app {

ZenithUI::ZenithUI(std::shared_ptr<AppState> state)
    : state_(std::move(state)),
      screen_(ftxui::ScreenInteractive::Fullscreen()) {
  ftxui::MenuOption option;
  option.on_change = [&] { screen_.Post(ftxui::Event::Custom); };
  option.entries_option.transform = [](const ftxui::EntryState& state) {
    auto element = ftxui::text(state.label);
    if (state.label == "No data available") return element;

    size_t last_pipe = state.label.find_last_of('|');
    if (last_pipe != std::string::npos) {
      std::string prefix = state.label.substr(0, last_pipe + 1);
      std::string suffix = state.label.substr(last_pipe + 1);

      bool is_rising = suffix.find("Rising") != std::string::npos ||
                       suffix.find("RISING") != std::string::npos;
      auto color = is_rising ? ftxui::Color::Green : ftxui::Color::Red;

      element = ftxui::hbox({
          ftxui::text(prefix),
          ftxui::text(suffix) | ftxui::color(color),
      });
    }

    if (state.focused) {
      element |= ftxui::focus;
      element |= ftxui::inverted;
    }
    return element;
  };
  star_menu_ = ftxui::Menu(&star_entries_, &star_selected_, option);
}

void ZenithUI::TriggerRefresh() { screen_.Post(ftxui::Event::Custom); }

void ZenithUI::Run() {
  auto renderer = ftxui::Renderer(star_menu_, [&] { return Render(); });

  auto event_handler = ftxui::CatchEvent(renderer, [&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q') ||
        event == ftxui::Event::Character('Q')) {
      screen_.Exit();
      state_->running = false;
      return true;
    }
    if (event == ftxui::Event::Character('f') ||
        event == ftxui::Event::Character('F')) {
      std::lock_guard<std::mutex> lock(state_->filter_mutex);
      state_->show_filter_window = !state_->show_filter_window;
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

  FilterCriteria filter;
  bool show_filter = false;
  {
    std::lock_guard<std::mutex> lock(state_->filter_mutex);
    filter = state_->filter;
    show_filter = state_->show_filter_window;
  }

  // Time Formatting
  std::string time_str = "N/A";
  if (time != std::chrono::system_clock::time_point{}) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm_now;
    gmtime_s(&tm_now, &time_t);
    time_str =
        std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02} UTC",
                    tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                    tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
  }

  bool gps_active = state_->gps_active.load();

  auto stars_solar_box = ftxui::vbox({
      RenderStars(stars, filter),
      RenderSolar(solar, filter),
  });

  auto main_content = ftxui::vflow({
      stars_solar_box,
      RenderRadar(stars, solar, filter),
      RenderSidebar(loc, gps_active, time_str),
  });

  if (show_filter) {
    return ftxui::dbox({
        main_content,
        RenderFilterWindow() | ftxui::clear_under | ftxui::center,
    });
  }

  return main_content;
}

ftxui::Element ZenithUI::RenderSidebar(const engine::Observer& loc,
                                       bool gps_active,
                                       const std::string& time_str) {
  return ftxui::vbox(
      {ftxui::window(
           ftxui::text(" Status "),
           ftxui::vbox({
               ftxui::text(
                   std::format("GPS: {}", gps_active ? "Active" : "Manual")) |
                   (gps_active ? ftxui::color(ftxui::Color::Green)
                               : ftxui::color(ftxui::Color::Yellow)),
               ftxui::text(std::format("Log: {}",
                                       state_->logging_enabled ? "On" : "Off")),
               ftxui::text("Time: " + time_str),
           })),
       ftxui::window(
           ftxui::text(" Location "),
           ftxui::vbox({
               ftxui::text(std::format("Lat: {:.4f} N", loc.latitude)),
               ftxui::text(std::format("Lon: {:.4f} E", loc.longitude)),
               ftxui::text(std::format("Alt: {:.1f} m", loc.altitude)),
           })),
       ftxui::text("Zenith Finder v0.4.0") | ftxui::dim | ftxui::center});
}

ftxui::Element ZenithUI::RenderStars(
    const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
    const FilterCriteria& filter) {
  star_entries_.clear();
  std::vector<engine::CelestialResult> filtered_stars;

  if (stars) {
    for (const auto& star : *stars) {
      // Apply Filter
      if (filter.active) {
        if (!filter.name_filter.empty()) {
          std::string name_lower = star.name;
          std::string filter_lower = filter.name_filter;
          std::transform(name_lower.begin(), name_lower.end(),
                         name_lower.begin(), ::tolower);
          std::transform(filter_lower.begin(), filter_lower.end(),
                         filter_lower.begin(), ::tolower);
          if (name_lower.find(filter_lower) == std::string::npos) continue;
        }
        if (star.elevation < filter.min_elevation ||
            star.elevation > filter.max_elevation)
          continue;
        if (star.azimuth < filter.min_azimuth ||
            star.azimuth > filter.max_azimuth)
          continue;
      }
      filtered_stars.push_back(star);
    }
  }

  // Populate menu entries
  for (const auto& star : filtered_stars) {
    std::string state_text = star.is_rising ? "Rising" : "Setting";
    std::string state_icon = star.is_rising ? std::string(ui::kIconRising)
                                            : std::string(ui::kIconSetting);
    star_entries_.push_back(std::format(
        "{:<15} | {:>9.5f} | {:>9.5f} | {:>9.3f} | {} {}", star.name,
        star.elevation, star.azimuth, star.magnitude, state_icon, state_text));
  }

  if (star_entries_.empty()) {
    star_entries_.push_back("No data available");
  }

  // Header
  auto header =
      ftxui::hbox({
          ftxui::text("Star") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 15),
          ftxui::text(" | "),
          ftxui::text("Elevation") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 9),
          ftxui::text(" | "),
          ftxui::text("Azimuth") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 9),
          ftxui::text(" | "),
          ftxui::text("Magnitude") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 9),
          ftxui::text(" | "),
          ftxui::text("State"),
      }) |
      ftxui::bold;

  std::string title = " Zenith Stars ";
  if (filter.active) title += "[Filtered] ";

  return ftxui::window(
             ftxui::text(title),
             ftxui::vbox({
                 header,
                 ftxui::separator(),
                 star_menu_->Render() | ftxui::vscroll_indicator | ftxui::frame,
             })) |
         ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 27);
}

ftxui::Element ZenithUI::RenderSolar(
    const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
    const FilterCriteria& filter) {
  std::vector<std::vector<ftxui::Element>> solar_rows = {
      {ftxui::text("Body") | ftxui::bold,
       ftxui::text("Elevation") | ftxui::bold,
       ftxui::text("Azimuth") | ftxui::bold,
       ftxui::text("Zenith") | ftxui::bold,
       ftxui::text("Dist (AU)") | ftxui::bold,
       ftxui::text("State") | ftxui::bold}};
  if (solar) {
    for (const auto& body : *solar) {
      // Apply Filter
      if (filter.active) {
        if (!filter.name_filter.empty()) {
          std::string name_lower = body.name;
          std::string filter_lower = filter.name_filter;
          std::transform(name_lower.begin(), name_lower.end(),
                         name_lower.begin(), ::tolower);
          std::transform(filter_lower.begin(), filter_lower.end(),
                         filter_lower.begin(), ::tolower);
          if (name_lower.find(filter_lower) == std::string::npos) continue;
        }
        if (body.elevation < filter.min_elevation ||
            body.elevation > filter.max_elevation)
          continue;
        if (body.azimuth < filter.min_azimuth ||
            body.azimuth > filter.max_azimuth)
          continue;
      }

      auto state_color =
          body.is_rising ? ftxui::Color::Green : ftxui::Color::Red;
      std::string state_text = body.is_rising ? "Rising" : "Setting";
      std::string state_icon = body.is_rising ? std::string(ui::kIconRising)
                                              : std::string(ui::kIconSetting);
      solar_rows.push_back({
          ftxui::text(body.name),
          ftxui::text(std::format("{:.5f}", body.elevation)),
          ftxui::text(std::format("{:.5f}", body.azimuth)),
          ftxui::text(std::format("{:.5f}", body.zenith_dist)),
          ftxui::text(std::format("{:.5f}", body.distance_au)),
          ftxui::hbox(
              {ftxui::text(state_icon + " "), ftxui::text(state_text)}) |
              ftxui::color(state_color),
      });
    }
  } else {
    solar_rows.push_back({ftxui::text("No data") | ftxui::dim});
  }

  auto solar_table = ftxui::Table(solar_rows);
  solar_table.SelectAll().Border(ftxui::LIGHT);
  solar_table.SelectRow(0).Decorate(ftxui::bold);
  solar_table.SelectRow(0).SeparatorVertical(ftxui::LIGHT);
  solar_table.SelectRow(0).Border(ftxui::DOUBLE);

  std::string title = " Solar System ";
  if (filter.active) title += "[Filtered] ";

  return ftxui::window(ftxui::text(title), solar_table.Render());
}

ftxui::Element ZenithUI::RenderRadar(
    const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
    const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
    const FilterCriteria& filter) {
  auto radar =
      ftxui::canvas(100, 100, [stars, solar, filter](ftxui::Canvas& c) {
        int cx = 50;
        int cy = 50;
        int r = 45;

        for (int i = 0; i < 360; i += 5) {
          double angle = i * std::numbers::pi / 180.0;
          int x1 = cx + static_cast<int>(r * std::cos(angle));
          int y1 = cy + static_cast<int>(r * std::sin(angle));
          int x2 = cx + static_cast<int>((r + 2) * std::cos(angle));
          int y2 = cy + static_cast<int>((r + 2) * std::sin(angle));
          c.DrawBlockLine(x1, y1, x2, y2, ftxui::Color::GrayDark);
        }

        c.DrawText(cx, cy - r - 5, "N");
        c.DrawText(cx + r + 5, cy, "E");
        c.DrawText(cx, cy + r + 5, "S");
        c.DrawText(cx - r - 8, cy, "W");

        if (stars) {
          for (const auto& star : *stars) {
            if (star.elevation < 0) continue;

            // Apply Filter
            if (filter.active) {
              if (!filter.name_filter.empty()) {
                std::string name_lower = star.name;
                std::string filter_lower = filter.name_filter;
                std::transform(name_lower.begin(), name_lower.end(),
                               name_lower.begin(), ::tolower);
                std::transform(filter_lower.begin(), filter_lower.end(),
                               filter_lower.begin(), ::tolower);
                if (name_lower.find(filter_lower) == std::string::npos)
                  continue;
              }
              if (star.elevation < filter.min_elevation ||
                  star.elevation > filter.max_elevation)
                continue;
              if (star.azimuth < filter.min_azimuth ||
                  star.azimuth > filter.max_azimuth)
                continue;
            }

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

        if (solar) {
          for (const auto& body : *solar) {
            if (body.elevation < 0) continue;

            // Apply Filter
            if (filter.active) {
              if (!filter.name_filter.empty()) {
                std::string name_lower = body.name;
                std::string filter_lower = filter.name_filter;
                std::transform(name_lower.begin(), name_lower.end(),
                               name_lower.begin(), ::tolower);
                std::transform(filter_lower.begin(), filter_lower.end(),
                               filter_lower.begin(), ::tolower);
                if (name_lower.find(filter_lower) == std::string::npos)
                  continue;
              }
              if (body.elevation < filter.min_elevation ||
                  body.elevation > filter.max_elevation)
                continue;
              if (body.azimuth < filter.min_azimuth ||
                  body.azimuth > filter.max_azimuth)
                continue;
            }

            double r_b = r * (body.zenith_dist / 90.0);
            double az_rad = (body.azimuth - 90.0) * std::numbers::pi / 180.0;
            int bx = cx + static_cast<int>(r_b * std::cos(az_rad));
            int by = cy + static_cast<int>(r_b * std::sin(az_rad));

            auto color = (body.name == "SUN") ? ftxui::Color::Yellow
                                              : ftxui::Color::Cyan;
            c.DrawBlockCircle(bx, by, 3, color);
            c.DrawText(bx + 2, by + 2, body.name);
          }
        }
      });

  std::string title = " Zenith Radar ";
  if (filter.active) title += "[Filtered] ";

  return ftxui::window(ftxui::text(title), radar | ftxui::center);
}

ftxui::Element ZenithUI::RenderFilterWindow() {
  // NOTE: This is a placeholder. A full interactive filter would require
  // ftxui::Component for Input fields. For now, we'll display the current
  // filter state.

  FilterCriteria filter;
  {
    std::lock_guard<std::mutex> lock(state_->filter_mutex);
    filter = state_->filter;
  }

  auto content = ftxui::vbox({
      ftxui::text("Filter Settings (Press 'f' to close)"),
      ftxui::separator(),
      ftxui::hbox(ftxui::text("Name: "),
                  ftxui::text(filter.name_filter.empty() ? "None"
                                                         : filter.name_filter)),
      ftxui::hbox(
          ftxui::text("Elevation Range: "),
          ftxui::text(std::format("[{:.1f}, {:.1f}]", filter.min_elevation,
                                  filter.max_elevation))),
      ftxui::hbox(
          ftxui::text("Azimuth Range: "),
          ftxui::text(std::format("[{:.1f}, {:.1f}]", filter.min_azimuth,
                                  filter.max_azimuth))),
      ftxui::separator(),
      ftxui::text(std::format("Active: {}", filter.active ? "YES" : "NO")) |
          ftxui::color(filter.active ? ftxui::Color::Green : ftxui::Color::Red),
      ftxui::text("Interactive editing coming in v0.5") | ftxui::dim,
  });

  return ftxui::window(ftxui::text(" Filters "), content) |
         ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40);
}

}  // namespace app
