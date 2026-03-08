#ifndef ZENITH_FINDER_APP_ZENITH_UI_HPP_
#define ZENITH_FINDER_APP_ZENITH_UI_HPP_

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <string>
#include <vector>

#include "../app_state.hpp"

namespace app {

class ZenithUI {
 public:
  explicit ZenithUI(std::shared_ptr<AppState> state);
  void Run();
  void TriggerRefresh();

 private:
  std::shared_ptr<AppState> state_;
  ftxui::ScreenInteractive screen_;

  // Star list scrolling state
  int star_selected_ = 0;
  std::vector<std::string> star_entries_;
  ftxui::Component star_menu_;

  // Solar list scrolling state
  int solar_selected_ = 0;
  std::vector<std::string> solar_entries_;
  ftxui::Component solar_menu_;

  ftxui::Element Render();
  ftxui::Element RenderMainContent();

  ftxui::Element RenderSidebar(const engine::Observer& loc, bool gps_active,
                               const std::string& time_str);
  ftxui::Element RenderStars(
      const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
      const engine::FilterCriteria& filter, const engine::SortCriteria& sort);
  ftxui::Element RenderSolar(
      const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
      const engine::FilterCriteria& filter, const engine::SortCriteria& sort);
  ftxui::Element RenderRadar(
      const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
      const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
      const engine::FilterCriteria& filter);
  ftxui::Element RenderFilterWindow();

  ftxui::Element SortableHeader(const std::string& label,
                                engine::SortColumn col,
                                const engine::SortCriteria& current_sort,
                                int width);

  // Filter UI Components
  std::string name_filter_str_;
  std::string min_elevation_str_;
  std::string max_elevation_str_;
  std::string min_azimuth_str_;
  std::string max_azimuth_str_;

  ftxui::Component name_input_;
  ftxui::Component min_elevation_input_;
  ftxui::Component max_elevation_input_;
  ftxui::Component min_azimuth_input_;
  ftxui::Component max_azimuth_input_;
  ftxui::Component filter_active_checkbox_;
  ftxui::Component filter_container_;

  // Container members for stability
  ftxui::Component main_container_;
  ftxui::Component filter_window_container_;
  ftxui::Component tab_container_;
  int tab_index_ = 0;

  void UpdateFilterFromUI();
  void UpdateUIFromFilter();
};

}  // namespace app

#endif  // ZENITH_FINDER_APP_ZENITH_UI_HPP_
