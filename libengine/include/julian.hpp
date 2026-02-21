#ifndef LIBENGINE_INCLUDE_JULIAN_H_
#define LIBENGINE_INCLUDE_JULIAN_H_

#include <chrono>
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
  using std::chrono::November;
  using std::chrono::round;
  using std::chrono::sys_days;
  using namespace std::chrono_literals;  // NOLINT

  auto constexpr kEpoch = sys_days{November / 24 / -4713} + 12h;

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
  using std::chrono::November;
  using std::chrono::sys_days;
  using namespace std::chrono_literals;  // NOLINT

  auto constexpr kEpoch = sys_days{November / 24 / -4713} + 12h;
  // tp is duration from JD 0. Adding it to the sys_time of JD 0.
  return kEpoch + tp.time_since_epoch();
}

inline JulianClock::time_point JulianClock::now() noexcept {
  return std::chrono::clock_cast<JulianClock>(std::chrono::system_clock::now());
}

}  // namespace engine

#endif  // LIBENGINE_INCLUDE_JULIAN_H_