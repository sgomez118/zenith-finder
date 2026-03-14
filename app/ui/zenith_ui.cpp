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
  auto make_menu_option = [&] {
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
    return option;
  };

  star_menu_ = ftxui::Menu(&star_entries_, &star_selected_, make_menu_option());
  solar_menu_ =
      ftxui::Menu(&solar_entries_, &solar_selected_, make_menu_option());

  // Filter UI Initialization
  UpdateUIFromFilter();

  name_input_ = ftxui::Input(&name_filter_str_, "Filter by name...");
  min_elevation_input_ = ftxui::Input(&min_elevation_str_, "0.0");
  max_elevation_input_ = ftxui::Input(&max_elevation_str_, "90.0");
  min_azimuth_input_ = ftxui::Input(&min_azimuth_str_, "0.0");
  max_azimuth_input_ = ftxui::Input(&max_azimuth_str_, "360.0");

  ftxui::CheckboxOption checkbox_option;
  checkbox_option.on_change = [&] { UpdateFilterFromUI(); };
  filter_active_checkbox_ =
      ftxui::Checkbox("Active", &state_->filter.active, checkbox_option);

  filter_container_ = ftxui::Container::Vertical({
      name_input_,
      min_elevation_input_,
      max_elevation_input_,
      min_azimuth_input_,
      max_azimuth_input_,
      filter_active_checkbox_,
  });

  filter_window_container_ =
      ftxui::Renderer(filter_container_, [&] { return RenderFilterWindow(); });

  main_container_ = ftxui::Container::Vertical({
      star_menu_,
      solar_menu_,
  });

  main_container_ =
      ftxui::Renderer(main_container_, [&] { return RenderMainContent(); });

  tab_container_ = ftxui::Container::Tab(
      {
          main_container_,
          filter_window_container_,
      },
      &tab_index_);
}

void ZenithUI::UpdateFilterFromUI() {
  std::lock_guard<std::mutex> lock(state_->filter_mutex);
  state_->filter.name_filter = name_filter_str_;
  try {
    if (!min_elevation_str_.empty())
      state_->filter.min_elevation = std::stof(min_elevation_str_);
  } catch (...) {
  }
  try {
    if (!max_elevation_str_.empty())
      state_->filter.max_elevation = std::stof(max_elevation_str_);
  } catch (...) {
  }
  try {
    if (!min_azimuth_str_.empty())
      state_->filter.min_azimuth = std::stof(min_azimuth_str_);
  } catch (...) {
  }
  try {
    if (!max_azimuth_str_.empty())
      state_->filter.max_azimuth = std::stof(max_azimuth_str_);
  } catch (...) {
  }
}

void ZenithUI::UpdateUIFromFilter() {
  std::lock_guard<std::mutex> lock(state_->filter_mutex);
  name_filter_str_ = state_->filter.name_filter;
  min_elevation_str_ = std::format("{:.1f}", state_->filter.min_elevation);
  max_elevation_str_ = std::format("{:.1f}", state_->filter.max_elevation);
  min_azimuth_str_ = std::format("{:.1f}", state_->filter.min_azimuth);
  max_azimuth_str_ = std::format("{:.1f}", state_->filter.max_azimuth);
}

void ZenithUI::TriggerRefresh() { screen_.Post(ftxui::Event::Custom); }

void ZenithUI::Run() {
  auto renderer = ftxui::Renderer(tab_container_, [&] { return Render(); });

  auto event_handler = ftxui::CatchEvent(renderer, [&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q') ||
        event == ftxui::Event::Character('Q')) {
      screen_.Exit();
      state_->running = false;
      return true;
    }
    if (event == ftxui::Event::Character('f') ||
        event == ftxui::Event::Character('F')) {
      bool showing = false;
      {
        std::lock_guard<std::mutex> lock(state_->filter_mutex);
        state_->show_filter_window = !state_->show_filter_window;
        showing = state_->show_filter_window;
      }
      if (showing) {
        UpdateUIFromFilter();
        tab_index_ = 1;
      } else {
        tab_index_ = 0;
      }
      return true;
    }

    if (tab_index_ == 1) {
      return false;  // Let tab_container handle events
    }

    if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
      if (star_menu_->Focused()) {
        solar_menu_->TakeFocus();
      } else {
        star_menu_->TakeFocus();
      }
      return true;
    }

    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k')) {
      if (star_menu_->Focused()) {
        if (star_selected_ <= 0) {
          std::lock_guard<std::mutex> lock(state_->filter_mutex);
          if (state_->filter.star_offset > 0) {
            size_t step = 50;
            if (state_->filter.star_offset >= step) {
              state_->filter.star_offset -= step;
            } else {
              state_->filter.star_offset = 0;
            }
            state_->filter.star_limit = 100;
            star_selected_ = static_cast<int>(step);
          } else {
            star_selected_ = 0;
          }
          return true;  // Stay in star_menu
        }
      } else if (solar_menu_->Focused()) {
        if (solar_selected_ <= 0) {
          solar_selected_ = 0;
          return true;  // Stay in solar_menu
        }
      }
    }

    if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j')) {
      if (star_menu_->Focused()) {
        if (star_selected_ >= static_cast<int>(star_entries_.size()) - 1) {
          if (!star_entries_.empty() && star_entries_[0] != "No data available") {
            std::lock_guard<std::mutex> lock(state_->filter_mutex);
            size_t current_limit = state_->filter.star_limit;
            if (star_entries_.size() >= current_limit) {
              size_t step = 50;
              state_->filter.star_offset += step;
              state_->filter.star_limit = 100;
              star_selected_ = std::max(
                  0, static_cast<int>(star_entries_.size()) - static_cast<int>(step) - 1);
            } else {
              star_selected_ = static_cast<int>(star_entries_.size()) - 1;
            }
          } else {
            star_selected_ = 0;
          }
          return true;  // Stay in star_menu
        }
      } else if (solar_menu_->Focused()) {
        if (solar_selected_ >= static_cast<int>(solar_entries_.size()) - 1) {
          solar_selected_ = std::max(0, static_cast<int>(solar_entries_.size()) - 1);
          return true;  // Stay in solar_menu
        }
      }
    }

    auto handle_sort = [&](engine::SortCriteria& criteria,
                           engine::SortColumn col) {
      std::lock_guard<std::mutex> lock(state_->sort_mutex);
      if (criteria.column == col) {
        criteria.ascending = !criteria.ascending;
      } else {
        criteria.column = col;
        criteria.ascending = true;
      }
      return true;
    };

    if (event == ftxui::Event::Character('1'))
      return handle_sort(state_->star_sort, engine::SortColumn::NAME);
    if (event == ftxui::Event::Character('2'))
      return handle_sort(state_->star_sort, engine::SortColumn::ELEVATION);
    if (event == ftxui::Event::Character('3'))
      return handle_sort(state_->star_sort, engine::SortColumn::AZIMUTH);
    if (event == ftxui::Event::Character('4'))
      return handle_sort(state_->star_sort, engine::SortColumn::MAGNITUDE);
    if (event == ftxui::Event::Character('5'))
      return handle_sort(state_->star_sort, engine::SortColumn::STATE);

    if (event == ftxui::Event::Character('6'))
      return handle_sort(state_->solar_sort, engine::SortColumn::NAME);
    if (event == ftxui::Event::Character('7'))
      return handle_sort(state_->solar_sort, engine::SortColumn::ELEVATION);
    if (event == ftxui::Event::Character('8'))
      return handle_sort(state_->solar_sort, engine::SortColumn::AZIMUTH);
    if (event == ftxui::Event::Character('9'))
      return handle_sort(state_->solar_sort, engine::SortColumn::ZENITH);
    if (event == ftxui::Event::Character('0'))
      return handle_sort(state_->solar_sort, engine::SortColumn::DISTANCE);
    if (event == ftxui::Event::Character('-'))
      return handle_sort(state_->solar_sort, engine::SortColumn::STATE);

    return false;
  });

  screen_.Loop(event_handler);
}

ftxui::Element ZenithUI::Render() {
  if (tab_index_ == 1) {
    UpdateFilterFromUI();
  }

  if (tab_index_ == 1) {
    return ftxui::dbox({
        main_container_->Render(),
        filter_window_container_->Render() | ftxui::clear_under | ftxui::center,
    });
  }

  return main_container_->Render();
}

ftxui::Element ZenithUI::RenderMainContent() {
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

  engine::FilterCriteria filter;
  {
    std::lock_guard<std::mutex> lock(state_->filter_mutex);
    filter = state_->filter;
  }

  engine::SortCriteria star_sort;
  engine::SortCriteria solar_sort;
  {
    std::lock_guard<std::mutex> lock(state_->sort_mutex);
    star_sort = state_->star_sort;
    solar_sort = state_->solar_sort;
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
      RenderStars(stars, filter, star_sort),
      RenderSolar(solar, filter, solar_sort),
  });

  return ftxui::vflow({
      stars_solar_box,
      RenderRadar(stars, solar, filter),
      RenderSidebar(loc, gps_active, time_str),
  });
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
    const engine::FilterCriteria& filter, const engine::SortCriteria& sort) {
  star_entries_.clear();
  std::vector<engine::CelestialResult> results;

  if (stars) {
    results = *stars;
  }

  // Populate menu entries
  for (const auto& star : results) {
    std::string state_text = star.is_rising ? "Rising" : "Setting";
    std::string state_icon = star.is_rising ? std::string(ui::kIconRising)
                                            : std::string(ui::kIconSetting);
    star_entries_.push_back(std::format(
        "{:<15} | {:>11.5f} | {:>9.5f} | {:>11.3f} | {} {}", star.name,
        star.elevation, star.azimuth, star.magnitude, state_icon, state_text));
  }

  if (star_entries_.empty()) {
    star_entries_.push_back("No data available");
  }

  // Clamping selection
  if (star_selected_ >= static_cast<int>(star_entries_.size())) {
    star_selected_ = std::max(0, static_cast<int>(star_entries_.size()) - 1);
  }
  if (star_selected_ < 0) star_selected_ = 0;

  // Header
  auto header =
      ftxui::hbox({
          SortableHeader("Star", engine::SortColumn::NAME, sort, 15),
          ftxui::text(" | "),
          SortableHeader("Elevation", engine::SortColumn::ELEVATION, sort, 11),
          ftxui::text(" | "),
          SortableHeader("Azimuth", engine::SortColumn::AZIMUTH, sort, 9),
          ftxui::text(" | "),
          SortableHeader("Magnitude", engine::SortColumn::MAGNITUDE, sort, 11),
          ftxui::text(" | "),
          SortableHeader("State", engine::SortColumn::STATE, sort, 8),
      }) |
      ftxui::bold;

  std::string title = " Zenith Stars ";
  if (filter.active) title += "[Filtered] ";
  if (filter.star_offset > 0 || filter.star_limit > 0) title += "[Windowed] ";

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
    const engine::FilterCriteria& filter, const engine::SortCriteria& sort) {
  solar_entries_.clear();
  std::vector<engine::SolarBody> results;

  if (solar) {
    results = *solar;
  }

  // Populate menu entries
  for (const auto& body : results) {
    std::string state_text = body.is_rising ? "Rising" : "Setting";
    std::string state_icon = body.is_rising ? std::string(ui::kIconRising)
                                            : std::string(ui::kIconSetting);
    solar_entries_.push_back(std::format(
        "{:<15} | {:>11.5f} | {:>9.5f} | {:>8.5f} | {:>11.5f} | {} {}",
        body.name, body.elevation, body.azimuth, body.zenith_dist,
        body.distance_au, state_icon, state_text));
  }

  if (solar_entries_.empty()) {
    solar_entries_.push_back("No data available");
  }

  // Clamping selection
  if (solar_selected_ >= static_cast<int>(solar_entries_.size())) {
    solar_selected_ = std::max(0, static_cast<int>(solar_entries_.size()) - 1);
  }
  if (solar_selected_ < 0) solar_selected_ = 0;

  // Header
  auto header =
      ftxui::hbox({
          SortableHeader("Body", engine::SortColumn::NAME, sort, 15),
          ftxui::text(" | "),
          SortableHeader("Elevation", engine::SortColumn::ELEVATION, sort, 11),
          ftxui::text(" | "),
          SortableHeader("Azimuth", engine::SortColumn::AZIMUTH, sort, 9),
          ftxui::text(" | "),
          SortableHeader("Zenith", engine::SortColumn::ZENITH, sort, 8),
          ftxui::text(" | "),
          SortableHeader("Dist (AU)", engine::SortColumn::DISTANCE, sort, 11),
          ftxui::text(" | "),
          SortableHeader("State", engine::SortColumn::STATE, sort, 8),
      }) |
      ftxui::bold;

  std::string title = " Solar System ";
  if (filter.active) title += "[Filtered] ";

  return ftxui::window(ftxui::text(title),
                       ftxui::vbox({
                           header,
                           ftxui::separator(),
                           solar_menu_->Render() | ftxui::vscroll_indicator |
                               ftxui::frame,
                       })) |
         ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 15);
}

ftxui::Element ZenithUI::RenderRadar(
    const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
    const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
    const engine::FilterCriteria& filter) {
  auto radar = ftxui::canvas(
      100, 100, [this, stars, solar, filter](ftxui::Canvas& c) {
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

        if (stars && !stars->empty()) {
          int total_stars = static_cast<int>(stars->size());
          int start = 0;
          int end = total_stars;

          // Star table height is 23 items
          if (total_stars > 23) {
            int current_star_sel = std::clamp(star_selected_, 0, total_stars - 1);
            start = std::max(0, current_star_sel - 11);
            if (start + 23 > total_stars) {
              start = std::max(0, total_stars - 23);
            }
            end = std::min(total_stars, start + 23);
          }

          for (int i = start; i < end; ++i) {
            const auto& star = (*stars)[i];
            if (star.elevation < 0) continue;

            double r_s = r * (star.zenith_dist / 90.0);
            double az_rad = (star.azimuth - 90.0) * std::numbers::pi / 180.0;
            int sx = cx + static_cast<int>(r_s * std::cos(az_rad));
            int sy = cy + static_cast<int>(r_s * std::sin(az_rad));

            auto color = (i == star_selected_) ? ftxui::Color::Green
                                               : ftxui::Color::White;
            if (star.zenith_dist < 2.0) {
              c.DrawBlockCircle(sx, sy, 2, ftxui::Color::Yellow);
              if (i == star_selected_) {
                c.DrawBlockCircle(sx, sy, 3, ftxui::Color::Green);
              }
            } else {
              if (i == star_selected_) {
                c.DrawBlockCircle(sx, sy, 2, ftxui::Color::Green);
              } else {
                c.DrawBlock(sx, sy, true, color);
              }
            }
          }
        }

        if (solar && !solar->empty()) {
          int total_solar = static_cast<int>(solar->size());
          int start = 0;
          int end = total_solar;

          // Solar table height is ~11 items
          if (total_solar > 11) {
            int current_solar_sel = std::clamp(solar_selected_, 0, total_solar - 1);
            start = std::max(0, current_solar_sel - 5);
            if (start + 11 > total_solar) {
              start = std::max(0, total_solar - 11);
            }
            end = std::min(total_solar, start + 11);
          }

          for (int i = start; i < end; ++i) {
            const auto& body = (*solar)[i];
            if (body.elevation < 0) continue;

            double r_b = r * (body.zenith_dist / 90.0);
            double az_rad = (body.azimuth - 90.0) * std::numbers::pi / 180.0;
            int bx = cx + static_cast<int>(r_b * std::cos(az_rad));
            int by = cy + static_cast<int>(r_b * std::sin(az_rad));

            auto color = (body.name == "SUN") ? ftxui::Color::Yellow
                                              : ftxui::Color::Cyan;

            if (i == solar_selected_) {
              c.DrawBlockCircle(bx, by, 4, ftxui::Color::Green);
            }
            c.DrawBlockCircle(bx, by, 3, color);
            c.DrawText(bx + 2, by + 2, std::string(body.name));
          }
        }
      });

  std::string title = " Zenith Radar ";
  if (filter.active) title += "[Filtered] ";
  if ((stars && stars->size() > 23) || (solar && solar->size() > 11)) {
    title += "[Windowed] ";
  }

  return ftxui::window(ftxui::text(title), radar | ftxui::center);
}

ftxui::Element ZenithUI::RenderFilterWindow() {
  return ftxui::window(
             ftxui::text(" Filters "),
             ftxui::vbox({
                 ftxui::hbox(ftxui::text("Name:      "), name_input_->Render()),
                 ftxui::hbox(ftxui::text("Min Elev:  "),
                             min_elevation_input_->Render()),
                 ftxui::hbox(ftxui::text("Max Elev:  "),
                             max_elevation_input_->Render()),
                 ftxui::hbox(ftxui::text("Min Azim:  "),
                             min_azimuth_input_->Render()),
                 ftxui::hbox(ftxui::text("Max Azim:  "),
                             max_azimuth_input_->Render()),
                 ftxui::separator(),
                 filter_active_checkbox_->Render(),
                 ftxui::separator(),
                 ftxui::text("Press 'f' to close") | ftxui::dim | ftxui::center,
             })) |
         ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40);
}

ftxui::Element ZenithUI::SortableHeader(
    const std::string& label, engine::SortColumn col,
    const engine::SortCriteria& current_sort, int width) {
  std::string text = label;
  if (current_sort.column == col) {
    text += (current_sort.ascending ? " ▲" : " ▼");
  }
  return ftxui::text(text) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width);
}

}  // namespace app
