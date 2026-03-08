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

  ftxui::Element RenderSidebar(const engine::Observer& loc, bool gps_active,
                               const std::string& time_str);
  ftxui::Element RenderStars(
      const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
      const FilterCriteria& filter, const SortCriteria& sort);
  ftxui::Element RenderSolar(
      const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
      const FilterCriteria& filter, const SortCriteria& sort);
  ftxui::Element RenderRadar(
      const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
      const std::shared_ptr<std::vector<engine::SolarBody>>& solar,
      const FilterCriteria& filter);
  ftxui::Element RenderFilterWindow();

  ftxui::Element SortableHeader(const std::string& label, SortColumn col,
                                const SortCriteria& current_sort, int width);
};

}  // namespace app

#endif  // ZENITH_FINDER_APP_ZENITH_UI_HPP_
