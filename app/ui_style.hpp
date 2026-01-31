#ifndef APP_UI_STYLE_HPP_
#define APP_UI_STYLE_HPP_

#include <string_view>

namespace app::ui {

// ANSI Color Codes
constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view BOLD = "\033[1m";
constexpr std::string_view DIM = "\033[2m";

constexpr std::string_view RED = "\033[31m";
constexpr std::string_view GREEN = "\033[32m";
constexpr std::string_view YELLOW = "\033[33m";
constexpr std::string_view BLUE = "\033[34m";
constexpr std::string_view CYAN = "\033[36m";
constexpr std::string_view WHITE = "\033[37m";

// Backgrounds
constexpr std::string_view BG_BLUE = "\033[44m";

// Helpers
constexpr std::string_view ICON_RISING =
    "A";  // Simple ASCII for now, or use unicode if confident in terminal
constexpr std::string_view ICON_SETTING = "V";

}  // namespace app::ui

#endif  // APP_UI_STYLE_HPP_
