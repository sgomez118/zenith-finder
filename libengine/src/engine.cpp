#define _EXCLUDE_DEPRECATED
#include "engine.hpp"

#include <algorithm>
#include <chrono>
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
                   star.ra, star.dec, star.pmra, star.pmdec, star.parallax,
                   star.radial_velocity, &catalog_entry);

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
  static const struct {
    novas_planet id;
    const char* name;
  } planets[] = {{NOVAS_SUN, "SUN"},         {NOVAS_MERCURY, "MERCURY"},
                 {NOVAS_VENUS, "VENUS"},     {NOVAS_EARTH, "EARTH"},
                 {NOVAS_MARS, "MARS"},       {NOVAS_JUPITER, "JUPITER"},
                 {NOVAS_SATURN, "SATURN"},   {NOVAS_URANUS, "URANUS"},
                 {NOVAS_NEPTUNE, "NEPTUNE"}, {NOVAS_PLUTO, "PLUTO"},
                 {NOVAS_MOON, "MOON"}};

  prebuilt_->planets.clear();
  prebuilt_->planets.reserve(std::size(planets));

  for (const auto& planet : planets) {
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

std::vector<CelestialResult> AstrometryEngine::CalculateZenithProximity(
    const Observer& obs, const FilterCriteria& filter,
    std::chrono::system_clock::time_point time) const {
  if (!initialized_) {
    InitializeNovas();
  }

  std::vector<CelestialResult> results;
  if (!prebuilt_ || prebuilt_->stars.empty()) {
    return results;
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
    return results;  // Cannot calculate without a valid frame
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
    std::transform(filter_lower.begin(), filter_lower.end(),
                   filter_lower.begin(), ::tolower);
  }

  for (auto i : std::views::iota(0ULL, prebuilt_->stars.size())) {
    // Quick name filter check before expensive calculations
    if (filter.active && !filter_lower.empty()) {
      std::string name_lower = star_names_[i];
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                     ::tolower);
      if (name_lower.find(filter_lower) == std::string::npos) continue;
    }

    sky_pos star_position = {0};
    double az = 0, el = 0;

    // Apparent coordinates in system
    auto status =
        novas_sky_pos(&prebuilt_->stars[i], &frame, NOVAS_CIRS, &star_position);

    if (status != 0) {
      continue;
    }

    // Get local horizontal coordinates
    novas_app_to_hor(&frame, NOVAS_CIRS, star_position.ra, star_position.dec,
                     novas_standard_refraction, &az, &el);

    // Filter by elevation and azimuth
    if (filter.active) {
      if (el < filter.min_elevation || el > filter.max_elevation) continue;
      if (az < filter.min_azimuth || az > filter.max_azimuth) continue;
    } else {
      // Default: Filter out objects below the horizon
      if (el < 0) continue;
    }

    // Determine if the star is rising by comparing to the future frame
    bool rising = false;
    if (frame_future_status == 0) {
      double az_f = 0, el_f = 0;
      novas_app_to_hor(&frame_future, NOVAS_CIRS, star_position.ra,
                       star_position.dec, novas_standard_refraction, &az_f,
                       &el_f);
      rising = (el_f > el);
    }

    results.emplace_back(CelestialResult{
        .name = star_names_[i],
        .elevation = el,
        .azimuth = az,
        .zenith_dist = 90.0 - el,
        .magnitude = magnitudes_[i],
        .is_rising = rising,
    });
  }

  return results;
}

std::vector<SolarBody> AstrometryEngine::CalculateSolarSystem(
    const Observer& obs, const FilterCriteria& filter,
    std::chrono::system_clock::time_point time) const {
  if (!initialized_) {
    InitializeNovas();
  }

  std::vector<SolarBody> results;
  if (!prebuilt_ || prebuilt_->planets.empty()) {
    return results;
  }

  results.reserve(prebuilt_->planets.size());

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
    return results;
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
    std::transform(filter_lower.begin(), filter_lower.end(),
                   filter_lower.begin(), ::tolower);
  }

  for (auto i : std::views::iota(0ULL, prebuilt_->planets.size())) {
    // Quick name filter check
    if (filter.active && !filter_lower.empty()) {
      std::string name_lower = prebuilt_->planets[i].name;
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                     ::tolower);
      if (name_lower.find(filter_lower) == std::string::npos) continue;
    }

    sky_pos planet_position = {0};
    // Apparent coordinates in system
    auto status = novas_sky_pos(&prebuilt_->planets[i], &frame, NOVAS_CIRS,
                                &planet_position);

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

    results.emplace_back(SolarBody{
        .name = prebuilt_->planets[i].name,
        .elevation = el,
        .azimuth = az,
        .zenith_dist = 90.0 - el,
        .distance_au = planet_position.dis,
        .is_rising = rising,
    });
  }

  return results;
}

}  // namespace engine
