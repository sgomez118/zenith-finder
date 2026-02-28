#ifndef ZENITH_FINDER_LIBENGINE_INCLUDE_CONSTANTS_HPP_
#define ZENITH_FINDER_LIBENGINE_INCLUDE_CONSTANTS_HPP_

namespace engine {

/**
 * @brief Current number of leap seconds (TAI - UTC).
 * @note This should be updated when a new leap second is announced.
 * As of 2024, it is 37 seconds.
 */
constexpr double kLeapSeconds = 37.0;

/**
 * @brief Difference between UT1 and UTC (UT1 - UTC).
 * @note This value changes daily and should ideally be fetched from IERS.
 */
constexpr double kDUT1 = 0.1;

/**
 * @brief Earth polar offset x, e.g. from IERS Bulletin A
 * @note This value changes daily and should ideally be fetched from IERS.
 */
constexpr double kPolarOffsetX = 104.89;

/**
 * @brief Earth polar offset y, e.g. from IERS Bulletin A
 * @note This value changes daily and should ideally be fetched from IERS.
 */
constexpr double kPolarOffsetY = 387.83;

}  // namespace engine

#endif  // ZENITH_FINDER_LIBENGINE_INCLUDE_CONSTANTS_HPP_
