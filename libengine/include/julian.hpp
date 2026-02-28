#ifndef LIBENGINE_INCLUDE_JULIAN_HPP_
#define LIBENGINE_INCLUDE_JULIAN_HPP_

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ratio>
#include <type_traits>

namespace engine {

struct JulianClock;

template <class Duration>
using JulianTime = std::chrono::time_point<JulianClock, Duration>;

struct JulianClock {
  using rep = double;
  using period = std::chrono::days::period;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<JulianClock>;

  static constexpr bool is_steady = false;

  static time_point now() noexcept;

  static constexpr std::chrono::sys_time<std::chrono::hours>
  EpochAsSys() noexcept {
    using std::chrono::November;
    return std::chrono::sys_days{November / 24 / -4713} +
           std::chrono::hours{12};
  }

  template <class Duration>
  static auto from_sys(const std::chrono::sys_time<Duration>& tp) noexcept;

  template <class Duration>
  static auto to_sys(
      const std::chrono::time_point<JulianClock, Duration>& tp) noexcept;
};

template <class Duration>
auto JulianClock::from_sys(const std::chrono::sys_time<Duration>& tp) noexcept {
  using std::chrono::hours;
  using std::chrono::microseconds;
  using std::chrono::round;

  auto constexpr kEpoch = EpochAsSys();

  using Rep = typename Duration::rep;
  using Period = typename Duration::period;

  if constexpr (!std::is_floating_point_v<Rep> && (Period::den > 1000000)) {
    // Duration is more precise than microseconds (e.g. nanoseconds).
    // Falling back to microseconds to avoid 6700-year overflow in 64-bit
    // integer.
    return std::chrono::time_point<JulianClock, microseconds>{
        round<microseconds>(tp) - kEpoch};
  } else {
    using D = std::common_type_t<Duration, hours>;
    return std::chrono::time_point<JulianClock, D>{tp - kEpoch};
  }
}

template <class Duration>
auto JulianClock::to_sys(
    const std::chrono::time_point<JulianClock, Duration>& tp) noexcept {
  auto constexpr kEpoch = EpochAsSys();
  // tp is duration from JD 0. Adding it to the sys_time of JD 0.
  return kEpoch + tp.time_since_epoch();
}

inline JulianClock::time_point JulianClock::now() noexcept {
  return std::chrono::clock_cast<JulianClock>(std::chrono::system_clock::now());
}

struct JulianDay {
  int64_t day_number;
  double fraction;
};

// Splits a time point into Julian Day Number and fraction of day.
//
// This function calculates the Julian Day in the timescale of the provided
// clock.
//
// @tparam Clock The source clock (e.g., system_clock, utc_clock, tai_clock).
// @tparam Duration The duration type of the input time point.
// @param tp The time point to convert.
// @return JulianDay A struct containing the integer day number and fractional
// day.
template <typename Clock, typename Duration>
JulianDay GetJulianDayParts(std::chrono::time_point<Clock, Duration> tp) {
  auto epoch_in_clock =
      std::chrono::clock_cast<Clock>(JulianClock::EpochAsSys());
  auto duration_since_epoch = tp - epoch_in_clock;
  auto d = std::chrono::duration_cast<
      std::chrono::duration<double, std::chrono::days::period>>(
      duration_since_epoch);

  double whole;
  double fract = std::modf(d.count(), &whole);
  return JulianDay{static_cast<int64_t>(whole), fract};
}

}  // namespace engine

#endif  // LIBENGINE_INCLUDE_JULIAN_HPP_