#ifndef ZENITH_FINDER_APP_ZENITH_UI_HPP_
#define ZENITH_FINDER_APP_ZENITH_UI_HPP_

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>

#include "app_state.hpp"

namespace app {

class ZenithUI {
 public:
  explicit ZenithUI(std::shared_ptr<AppState> state);
  void Run();
  void TriggerRefresh();

 private:
  std::shared_ptr<AppState> state_;
  ftxui::ScreenInteractive screen_;

  ftxui::Element Render();

  ftxui::Element RenderSidebar(const engine::Observer& loc, bool gps_active,
                               const std::string& time_str);
  ftxui::Element RenderStars(
      const std::shared_ptr<std::vector<engine::CelestialResult>>& stars);
  ftxui::Element RenderSolar(
      const std::shared_ptr<std::vector<engine::SolarBody>>& solar);
  ftxui::Element RenderRadar(
      const std::shared_ptr<std::vector<engine::CelestialResult>>& stars,
      const std::shared_ptr<std::vector<engine::SolarBody>>& solar);
};

}  // namespace app

#endif  // ZENITH_FINDER_APP_ZENITH_UI_HPP_
