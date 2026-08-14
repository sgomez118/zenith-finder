#ifndef ZENITH_FINDER_LIBENGINE_INCLUDE_JULIAN_HPP_
#define ZENITH_FINDER_LIBENGINE_INCLUDE_JULIAN_HPP_

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ratio>
#include <type_traits>

namespace engine {

struct JulianClock;

/**
 * @brief Represents a time point in the Julian calendar.
 *
 * This uses JulianClock, which has its epoch at JD 0.0 (4714 BC Nov 24 12:00:00
 * UTC).
 */
template <class Duration>
using JulianTime = std::chrono::time_point<JulianClock, Duration>;

/**
 * @brief A C++20 compliant clock for Julian Days.
 *
 * The epoch (JD 0.0) is 4714 BC November 24 at 12:00:00 UTC.
 */
struct JulianClock {
  using rep = double;
  using period = std::chrono::days::period;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<JulianClock>;

  static constexpr bool is_steady = false;

  template <class Duration>
  using from_sys_duration_t =
      std::conditional_t<!std::is_floating_point_v<typename Duration::rep> &&
                             (Duration::period::den > 1000000),
                         std::chrono::microseconds,
                         std::common_type_t<Duration, std::chrono::hours>>;

  template <class Duration>
  using from_sys_t = JulianTime<from_sys_duration_t<Duration>>;

  /**
   * @brief Returns the current time as a Julian time point.
   */
  static time_point now() noexcept {
    return from_sys(std::chrono::system_clock::now());
  }

  /**
   * @brief Returns the epoch of the Julian clock as a system time point.
   *
   * JD 0.0 corresponds to 4714 BC-11-24 12:00:00 UTC.
   * In ISO 8601/C++20 chrono terms, 4714 BC is year -4713.
   */
  static constexpr std::chrono::sys_time<std::chrono::hours>
  EpochAsSys() noexcept {
    return std::chrono::sys_days{std::chrono::November / 24 / -4713} +
           std::chrono::hours{12};
  }

  /**
   * @brief Converts a system time point to a Julian clock time point.
   */
  template <class Duration>
  static from_sys_t<Duration> from_sys(
      const std::chrono::sys_time<Duration>& tp) noexcept {
    auto constexpr kEpoch = EpochAsSys();

    using Rep = typename Duration::rep;
    using Period = typename Duration::period;

    if constexpr (!std::is_floating_point_v<Rep> && (Period::den > 1000000)) {
      // For high-precision integral durations (e.g., nanoseconds),
      // convert to microseconds to avoid overflow in the 6700-year range
      // when calculating the duration from the Julian epoch.
      return std::chrono::time_point_cast<std::chrono::microseconds>(
          JulianTime<std::chrono::microseconds>{
              std::chrono::round<std::chrono::microseconds>(tp - kEpoch)});
    } else {
      using D = std::common_type_t<Duration, std::chrono::hours>;
      return JulianTime<D>{tp - kEpoch};
    }
  }

  /**
   * @brief Converts a Julian clock time point to a system time point.
   */
  template <class Duration>
  static auto to_sys(
      const std::chrono::time_point<JulianClock, Duration>& tp) noexcept
      -> std::chrono::time_point<
          std::chrono::system_clock,
          std::common_type_t<std::chrono::hours, Duration>> {
    return EpochAsSys() + tp.time_since_epoch();
  }
};

/**
 * @brief Represents a Julian Day broken into its integer and fractional parts.
 */
struct JulianDay {
  int64_t day_number;
  double fraction;
};

/**
 * @brief Splits a time point into Julian Day Number and fraction of day.
 *
 * This function calculates the Julian Day in the timescale of the provided
 * clock.
 *
 * @tparam Clock The source clock (e.g., system_clock, utc_clock).
 * @tparam Duration The duration type of the input time point.
 * @param tp The time point to convert.
 * @return JulianDay A struct containing the integer day number and fractional
 * day.
 */
template <typename Clock, typename Duration>
JulianDay GetJulianDayParts(std::chrono::time_point<Clock, Duration> tp) {
  // Convert the Julian epoch to the target clock's timescale.
  // Cast to microseconds to avoid integer overflow in 64-bit nanoseconds over
  // the 6700-year span.
  auto tp_us = std::chrono::time_point_cast<std::chrono::microseconds>(tp);
  auto epoch_in_clock = std::chrono::time_point_cast<std::chrono::microseconds>(
      std::chrono::clock_cast<Clock>(JulianClock::EpochAsSys()));
  auto duration_since_epoch = tp_us - epoch_in_clock;

  // Convert to fractional days.
  auto d = std::chrono::duration_cast<
      std::chrono::duration<double, std::chrono::days::period>>(
      duration_since_epoch);

  double whole;
  double fract = std::modf(d.count(), &whole);

  // Adjust for negative fractions to ensure the fraction is always [0, 1).
  if (fract < 0) {
    fract += 1.0;
    whole -= 1.0;
  }

  return JulianDay{static_cast<int64_t>(whole), fract};
}

}  // namespace engine

#endif  // ZENITH_FINDER_LIBENGINE_INCLUDE_JULIAN_HPP_
