#define _EXCLUDE_DEPRECATED
#include "engine.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <execution>
#include <filesystem>
#include <format>
#include <iostream>
#include <mutex>
#include <ranges>

extern "C" {
#include <novas-calceph.h>
#include <novas.h>
}

#include "constants.hpp"
#include "julian.hpp"

namespace engine {

struct AstrometryEngine::PrebuiltCatalog {
  std::vector<cat_entry> star_entries;
  std::vector<object> stars;
  std::vector<object> planets;
};

AstrometryEngine::AstrometryEngine()
    : prebuilt_(std::make_unique<PrebuiltCatalog>()) {}

AstrometryEngine::~AstrometryEngine() = default;

void AstrometryEngine::SetCatalog(std::span<const Star> catalog) {
  star_names_.clear();
  magnitudes_.clear();
  prebuilt_->stars.clear();
  prebuilt_->star_entries.clear();

  star_names_.reserve(catalog.size());
  magnitudes_.reserve(catalog.size());
  prebuilt_->star_entries.reserve(catalog.size());
  prebuilt_->stars.reserve(catalog.size());

  for (const auto& star : catalog) {
    cat_entry catalog_entry;

    // Truncate strings to fit NOVAS buffers (names are 51 chars max)
    std::string safe_name = star.name.substr(0, 50);
    std::string safe_cat = star.catalog.substr(0, 3);

    // Define ICRS coordinates
    make_cat_entry(safe_name.c_str(), safe_cat.c_str(), star.catalog_id,
                   star.ra * kDegToHours, star.dec, star.pmra, star.pmdec,
                   star.parallax, star.radial_velocity, &catalog_entry);

    prebuilt_->star_entries.push_back(catalog_entry);

    object star_object;
    // Note: make_cat_object stores the pointer to the entry, so we must point
    // to the persistent vector
    make_cat_object(&prebuilt_->star_entries.back(), &star_object);

    star_names_.push_back(star.name);
    magnitudes_.push_back(star.flux);
    prebuilt_->stars.push_back(star_object);
  }
}

void AstrometryEngine::BuildPlanetsCatalog() const {
  struct PlanetInfo {
    novas_planet id;
    const char* name;
  };
  static constexpr PlanetInfo kPlanets[] = {
      {NOVAS_SUN, "SUN"},         {NOVAS_MERCURY, "MERCURY"},
      {NOVAS_VENUS, "VENUS"},     {NOVAS_EARTH, "EARTH"},
      {NOVAS_MARS, "MARS"},       {NOVAS_JUPITER, "JUPITER"},
      {NOVAS_SATURN, "SATURN"},   {NOVAS_URANUS, "URANUS"},
      {NOVAS_NEPTUNE, "NEPTUNE"}, {NOVAS_PLUTO, "PLUTO"},
      {NOVAS_MOON, "MOON"}};

  prebuilt_->planets.clear();
  prebuilt_->planets.reserve(std::size(kPlanets));

  for (const auto& planet : kPlanets) {
    object planet_object;
    make_planet(planet.id, &planet_object);
    // Ensure the name is set in the object struct and truncated safely
    std::strncpy(planet_object.name, planet.name,
                 sizeof(planet_object.name) - 1);
    planet_object.name[sizeof(planet_object.name) - 1] = '\0';
    prebuilt_->planets.push_back(planet_object);
  }
}

void AstrometryEngine::SetEphemeris(std::shared_ptr<t_calcephbin> ephemeris) {
  ephemeris_ = std::move(ephemeris);
  initialized_ = false;  // Force re-initialization of NOVAS
}

void AstrometryEngine::InitializeNovas() const {
  std::lock_guard<std::mutex> lock(initialization_mutex_);
  if (initialized_) return;

  if (ephemeris_) {
    auto result = novas_use_calceph(ephemeris_.get());
    if (result < 0) {
      accuracy_ = NOVAS_REDUCED_ACCURACY;
    } else {
      accuracy_ = NOVAS_FULL_ACCURACY;
    }
  } else {
    accuracy_ = NOVAS_REDUCED_ACCURACY;
  }
  initialized_ = true;

  BuildPlanetsCatalog();
}

namespace {
bool CaseInsensitiveContains(std::string_view haystack,
                             std::string_view needle_lower) {
  if (needle_lower.empty()) return true;
  auto it =
      std::search(haystack.begin(), haystack.end(), needle_lower.begin(),
                  needle_lower.end(), [](unsigned char a, unsigned char b) {
                    return std::tolower(a) == static_cast<int>(b);
                  });
  return it != haystack.end();
}
}  // namespace

std::vector<CelestialResult> AstrometryEngine::CalculateZenithProximity(
    const Observer& obs, const FilterCriteria& filter, const SortCriteria& sort,
    std::chrono::system_clock::time_point time) const {
  ResultBuffer buffer;
  CalculateZenithProximity(buffer, obs, filter, sort, time);
  return std::move(buffer.star_results);
}

void AstrometryEngine::CalculateZenithProximity(
    ResultBuffer& buffer, const Observer& obs, const FilterCriteria& filter,
    const SortCriteria& sort,
    std::chrono::system_clock::time_point time) const {
  if (!initialized_) {
    InitializeNovas();
  }

  buffer.star_results.clear();
  if (!prebuilt_ || prebuilt_->stars.empty()) {
    return;
  }

  observer location;
  novas_timespec t_spec;
  novas_frame frame;

  // Observer location
  make_gps_observer(obs.latitude, obs.longitude, obs.altitude, &location);

  // Set time of observation
  auto jd = GetJulianDayParts(time);
  novas_set_split_time(NOVAS_UTC, jd.day_number, jd.fraction, kLeapSeconds,
                       kDUT1, &t_spec);

  // Observer frame
  auto frame_status =
      novas_make_frame(static_cast<novas_accuracy>(accuracy_), &location,
                       &t_spec, kPolarOffsetX, kPolarOffsetY, &frame);

  if (frame_status != 0) {
    return;
  }

  // Prepare a future frame to determine if objects are rising or setting
  novas_timespec t_spec_future;
  novas_frame frame_future;
  auto jd_future = GetJulianDayParts(time + std::chrono::minutes(1));
  novas_set_split_time(NOVAS_UTC, jd_future.day_number, jd_future.fraction,
                       kLeapSeconds, kDUT1, &t_spec_future);
  auto frame_future_status = novas_make_frame(
      static_cast<novas_accuracy>(accuracy_), &location, &t_spec_future,
      kPolarOffsetX, kPolarOffsetY, &frame_future);

  std::string filter_lower = filter.name_filter;
  if (filter.active && !filter_lower.empty()) {
    std::ranges::transform(filter_lower, filter_lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
  }

  // Use a thread-local or pre-allocated vector for intermediate results to
  // avoid heap churn. For now, we still use a local vector but we can optimize
  // further if needed.
  std::vector<std::optional<CelestialResult>> all_results(
      prebuilt_->stars.size());
  auto indices = std::views::iota(size_t{0}, prebuilt_->stars.size());

  std::for_each(
      std::execution::par, indices.begin(), indices.end(), [&](size_t i) {
        // Quick name filter check before expensive calculations
        if (filter.active && !filter_lower.empty()) {
          if (!CaseInsensitiveContains(star_names_[i], filter_lower)) return;
        }

        novas_frame frame_local = frame;
        sky_pos star_position = {0};
        double az = 0, el = 0;

        // Apparent coordinates in system
        auto status = novas_sky_pos(&prebuilt_->stars[i], &frame_local,
                                    NOVAS_CIRS, &star_position);

        if (status != 0) {
          return;
        }

        // Get local horizontal coordinates
        novas_app_to_hor(&frame_local, NOVAS_CIRS, star_position.ra,
                         star_position.dec, novas_standard_refraction, &az,
                         &el);

        // Filter by elevation and azimuth
        if (filter.active) {
          if (el < filter.min_elevation || el > filter.max_elevation) return;
          if (az < filter.min_azimuth || az > filter.max_azimuth) return;
        } else {
          // Default: Filter out objects below the horizon
          if (el < 0) return;
        }

        // Determine if the star is rising by comparing to the future frame
        bool rising = false;
        if (frame_future_status == 0) {
          novas_frame frame_future_local = frame_future;
          double az_f = 0, el_f = 0;
          novas_app_to_hor(&frame_future_local, NOVAS_CIRS, star_position.ra,
                           star_position.dec, novas_standard_refraction, &az_f,
                           &el_f);
          rising = (el_f > el);
        }

        all_results[i] = CelestialResult{
            .name = star_names_[i],
            .elevation = el,
            .azimuth = az,
            .zenith_dist = 90.0 - el,
            .magnitude = magnitudes_[i],
            .is_rising = rising,
        };
      });

  // Collect results into the provided buffer
  for (auto& res_opt : all_results) {
    if (res_opt) {
      buffer.star_results.push_back(*res_opt);
    }
  }

  // Sorting
  if (sort.column != SortColumn::NONE) {
    std::stable_sort(
        buffer.star_results.begin(), buffer.star_results.end(),
        [&](const CelestialResult& a, const CelestialResult& b) {
          auto get_val = [&](const CelestialResult& res) {
            switch (sort.column) {
              case SortColumn::NAME:
                return 0.0;  // Special case for string
              case SortColumn::ELEVATION:
                return res.elevation;
              case SortColumn::AZIMUTH:
                return res.azimuth;
              case SortColumn::MAGNITUDE:
                return static_cast<double>(res.magnitude);
              case SortColumn::STATE:
                return static_cast<double>(res.is_rising);
              default:
                return 0.0;
            }
          };

          if (sort.column == SortColumn::NAME) {
            return sort.ascending ? (a.name < b.name) : (b.name < a.name);
          }

          double val_a = get_val(a);
          double val_b = get_val(b);
          return sort.ascending ? (val_a < val_b) : (val_b < val_a);
        });
  }

  // Apply offset and limit
  if (filter.star_offset > 0 || filter.star_limit > 0) {
    if (filter.star_offset >= buffer.star_results.size()) {
      buffer.star_results.clear();
      return;
    }
    auto start = buffer.star_results.begin() + filter.star_offset;
    auto end = buffer.star_results.end();
    if (filter.star_limit > 0 &&
        filter.star_limit < static_cast<size_t>(std::distance(
                                start, buffer.star_results.end()))) {
      end = start + filter.star_limit;
    }

    // Move the relevant slice to the front and resize
    if (start != buffer.star_results.begin()) {
      std::move(start, end, buffer.star_results.begin());
    }
    buffer.star_results.erase(
        buffer.star_results.begin() + std::distance(start, end),
        buffer.star_results.end());
  }
}

std::vector<SolarBody> AstrometryEngine::CalculateSolarSystem(
    const Observer& obs, const FilterCriteria& filter, const SortCriteria& sort,
    std::chrono::system_clock::time_point time) const {
  ResultBuffer buffer;
  CalculateSolarSystem(buffer, obs, filter, sort, time);
  return std::move(buffer.solar_results);
}

void AstrometryEngine::CalculateSolarSystem(
    ResultBuffer& buffer, const Observer& obs, const FilterCriteria& filter,
    const SortCriteria& sort,
    std::chrono::system_clock::time_point time) const {
  if (!initialized_) {
    InitializeNovas();
  }

  buffer.solar_results.clear();
  if (!prebuilt_ || prebuilt_->planets.empty()) {
    return;
  }

  observer location;
  novas_timespec t_spec;
  novas_frame frame;

  // Observer location
  make_gps_observer(obs.latitude, obs.longitude, obs.altitude, &location);

  // Set time of observance
  auto jd = GetJulianDayParts(time);
  novas_set_split_time(NOVAS_UTC, jd.day_number, jd.fraction, kLeapSeconds,
                       kDUT1, &t_spec);

  // Observer frame
  auto frame_status =
      novas_make_frame(static_cast<novas_accuracy>(accuracy_), &location,
                       &t_spec, kPolarOffsetX, kPolarOffsetY, &frame);

  if (frame_status != 0) {
    return;
  }

  // Prepare a future frame to determine if objects are rising or setting
  novas_timespec t_spec_future;
  novas_frame frame_future;
  auto jd_future = GetJulianDayParts(time + std::chrono::seconds(1));
  novas_set_split_time(NOVAS_UTC, jd_future.day_number, jd_future.fraction,
                       kLeapSeconds, kDUT1, &t_spec_future);
  auto frame_future_status = novas_make_frame(
      static_cast<novas_accuracy>(accuracy_), &location, &t_spec_future,
      kPolarOffsetX, kPolarOffsetY, &frame_future);

  std::string filter_lower = filter.name_filter;
  if (filter.active && !filter_lower.empty()) {
    std::ranges::transform(filter_lower, filter_lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });
  }

  for (const auto& planet_obj : prebuilt_->planets) {
    // Quick name filter check
    if (filter.active && !filter_lower.empty()) {
      if (!CaseInsensitiveContains(planet_obj.name, filter_lower)) continue;
    }

    sky_pos planet_position = {0};
    // Apparent coordinates in system
    auto status =
        novas_sky_pos(&planet_obj, &frame, NOVAS_CIRS, &planet_position);

    if (status != 0) continue;

    // Get local horizontal coordinates
    double az = 0, el = 0;
    novas_app_to_hor(&frame, NOVAS_CIRS, planet_position.ra,
                     planet_position.dec, novas_standard_refraction, &az, &el);

    // Filter by elevation and azimuth
    if (filter.active) {
      if (el < filter.min_elevation || el > filter.max_elevation) continue;
      if (az < filter.min_azimuth || az > filter.max_azimuth) continue;
    } else {
      // Default: Filter out objects below the horizon
      if (el < 0) continue;
    }

    // Determine if the planet is rising by comparing to the future frame
    bool rising = false;
    if (frame_future_status == 0) {
      double az_f = 0, el_f = 0;
      novas_app_to_hor(&frame_future, NOVAS_CIRS, planet_position.ra,
                       planet_position.dec, novas_standard_refraction, &az_f,
                       &el_f);
      rising = (el_f > el);
    }

    buffer.solar_results.emplace_back(SolarBody{
        .name = planet_obj.name,
        .elevation = el,
        .azimuth = az,
        .zenith_dist = 90.0 - el,
        .distance_au = planet_position.dis,
        .is_rising = rising,
    });
  }

  // Sorting
  if (sort.column != SortColumn::NONE) {
    std::stable_sort(
        buffer.solar_results.begin(), buffer.solar_results.end(),
        [&](const SolarBody& a, const SolarBody& b) {
          auto get_val = [&](const SolarBody& res) {
            switch (sort.column) {
              case SortColumn::NAME:
                return 0.0;
              case SortColumn::ELEVATION:
                return res.elevation;
              case SortColumn::AZIMUTH:
                return res.azimuth;
              case SortColumn::ZENITH:
                return res.zenith_dist;
              case SortColumn::DISTANCE:
                return res.distance_au;
              case SortColumn::STATE:
                return static_cast<double>(res.is_rising);
              default:
                return 0.0;
            }
          };

          if (sort.column == SortColumn::NAME) {
            return sort.ascending ? (a.name < b.name) : (b.name < a.name);
          }

          double val_a = get_val(a);
          double val_b = get_val(b);
          return sort.ascending ? (val_a < val_b) : (val_b < val_a);
        });
  }

  // Apply offset and limit
  if (filter.solar_offset > 0 || filter.solar_limit > 0) {
    if (filter.solar_offset >= buffer.solar_results.size()) {
      buffer.solar_results.clear();
      return;
    }
    auto start = buffer.solar_results.begin() + filter.solar_offset;
    auto end = buffer.solar_results.end();
    if (filter.solar_limit > 0 &&
        filter.solar_limit < static_cast<size_t>(std::distance(
                                 start, buffer.solar_results.end()))) {
      end = start + filter.solar_limit;
    }

    // Move the relevant slice to the front and resize
    if (start != buffer.solar_results.begin()) {
      std::move(start, end, buffer.solar_results.begin());
    }
    buffer.solar_results.erase(
        buffer.solar_results.begin() + std::distance(start, end),
        buffer.solar_results.end());
  }
}

}  // namespace engine
