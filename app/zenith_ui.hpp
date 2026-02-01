#ifndef APP_ZENITH_UI_HPP_
#define APP_ZENITH_UI_HPP_

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
};

}  // namespace app

#endif  // APP_ZENITH_UI_HPP_
