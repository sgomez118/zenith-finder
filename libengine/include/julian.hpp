#ifndef LIBENGINE_INCLUDE_JULIAN_HPP_
#define LIBENGINE_INCLUDE_JULIAN_HPP_

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
 * This uses JulianClock, which has its epoch at JD 0.0 (4714 BC Nov 24 12:00:00 UTC).
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

  /**
   * @brief Returns the current time as a Julian time point.
   */
  static time_point now() noexcept {
    return std::chrono::clock_cast<JulianClock>(std::chrono::system_clock::now());
  }

  /**
   * @brief Returns the epoch of the Julian clock as a system time point.
   *
   * JD 0.0 corresponds to 4714 BC-11-24 12:00:00 UTC.
   * In ISO 8601/C++20 chrono terms, 4714 BC is year -4713.
   */
  static constexpr std::chrono::sys_time<std::chrono::hours> EpochAsSys() noexcept {
    using namespace std::chrono;
    return sys_days{November / 24 / -4713} + 12h;
  }

  /**
   * @brief Converts a system time point to a Julian clock time point.
   */
  template <class Duration>
  static auto from_sys(const std::chrono::sys_time<Duration>& tp) noexcept {
    using namespace std::chrono;
    auto constexpr kEpoch = EpochAsSys();

    using Rep = typename Duration::rep;
    using Period = typename Duration::period;

    if constexpr (!std::is_floating_point_v<Rep> && (Period::den > 1000000)) {
      // For high-precision integral durations (e.g., nanoseconds), 
      // convert to microseconds to avoid overflow in the 6700-year range 
      // when calculating the duration from the Julian epoch.
      return time_point_cast<microseconds>(
          JulianTime<microseconds>{round<microseconds>(tp - kEpoch)});
    } else {
      using D = std::common_type_t<Duration, hours>;
      return JulianTime<D>{tp - kEpoch};
    }
  }

  /**
   * @brief Converts a Julian clock time point to a system time point.
   */
  template <class Duration>
  static auto to_sys(const std::chrono::time_point<JulianClock, Duration>& tp) noexcept {
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
 * This function calculates the Julian Day in the timescale of the provided clock.
 *
 * @tparam Clock The source clock (e.g., system_clock, utc_clock).
 * @tparam Duration The duration type of the input time point.
 * @param tp The time point to convert.
 * @return JulianDay A struct containing the integer day number and fractional day.
 */
template <typename Clock, typename Duration>
JulianDay GetJulianDayParts(std::chrono::time_point<Clock, Duration> tp) {
  using namespace std::chrono;
  
  // Convert the Julian epoch to the target clock's timescale.
  auto epoch_in_clock = clock_cast<Clock>(JulianClock::EpochAsSys());
  auto duration_since_epoch = tp - epoch_in_clock;
  
  // Convert to fractional days.
  auto d = duration_cast<duration<double, days::period>>(duration_since_epoch);

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

#endif  // LIBENGINE_INCLUDE_JULIAN_HPP_
