#ifndef ZENITH_FINDER_APP_UI_STYLE_HPP_
#define ZENITH_FINDER_APP_UI_STYLE_HPP_

#include <string_view>

namespace app::ui {

// ANSI Color Codes
constexpr std::string_view kReset = "\033[0m";
constexpr std::string_view kBold = "\033[1m";
constexpr std::string_view kDim = "\033[2m";

constexpr std::string_view kRed = "\033[31m";
constexpr std::string_view kGreen = "\033[32m";
constexpr std::string_view kYellow = "\033[33m";
constexpr std::string_view kBlue = "\033[34m";
constexpr std::string_view kCyan = "\033[36m";
constexpr std::string_view kWhite = "\033[37m";

// Backgrounds
constexpr std::string_view kBgBlue = "\033[44m";

// Helpers
constexpr std::string_view kIconRising =
    "A";  // Simple ASCII for now, or use unicode if confident in terminal
constexpr std::string_view kIconSetting = "V";

}  // namespace app::ui

#endif  // ZENITH_FINDER_APP_UI_STYLE_HPP_
