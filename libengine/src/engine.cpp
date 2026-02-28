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
  std::vector<object> stars;
};

AstrometryEngine::AstrometryEngine()
    : prebuilt_(std::make_unique<PrebuiltCatalog>()) {}

AstrometryEngine::~AstrometryEngine() = default;

void AstrometryEngine::SetCatalog(std::span<const Star> catalog) {
  star_names_.clear();
  prebuilt_->stars.clear();
  star_names_.reserve(catalog.size());
  prebuilt_->stars.reserve(catalog.size());

  for (const auto& star : catalog) {
    cat_entry catalog_entry;
    object star_object;

    // Define ICRS coordinates
    make_cat_entry(star.name.c_str(), star.catalog.c_str(), star.catalog_id,
                   star.ra, star.dec, star.pmra, star.pmdec, star.parallax,
                   star.radial_velocity, &catalog_entry);
    make_cat_object(&catalog_entry, &star_object);

    star_names_.push_back(star.name);
    prebuilt_->stars.push_back(star_object);
  }
}

void AstrometryEngine::SetEphemeris(std::shared_ptr<t_calcephbin> ephemeris) {
  ephemeris_ = std::move(ephemeris);
  initialized_ = false;  // Force re-initialization of NOVAS
}

void AstrometryEngine::InitializeNovas() const {
  auto result = novas_use_calceph(ephemeris_.get());
  if (result < 0) {
    accuracy_ = NOVAS_REDUCED_ACCURACY;
  } else {
    accuracy_ = NOVAS_FULL_ACCURACY;
  }
  initialized_ = true;
}

std::vector<CelestialResult> AstrometryEngine::CalculateZenithProximity(
    const Observer& obs, std::chrono::system_clock::time_point time) const {
  InitializeNovas();

  std::vector<CelestialResult> results;
  if (prebuilt_->stars.empty()) {
    return results;
  }

  results.reserve(prebuilt_->stars.size());

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
  novas_make_frame(static_cast<novas_accuracy>(accuracy_), &location, &t_spec,
                   kPolarOffsetX, kPolarOffsetY, &frame);

  for (auto i : std::views::iota(0ULL, prebuilt_->stars.size())) {
    sky_pos star_position;

    // Apparent coordinates in system
    novas_sky_pos(&prebuilt_->stars[i], &frame, NOVAS_CIRS, &star_position);

    // Get local horizontal coordinates
    double az, el;
    novas_app_to_hor(&frame, NOVAS_CIRS, star_position.ra, star_position.dec,
                     novas_standard_refraction, &az, &el);

    results.emplace_back(CelestialResult{
        .name = star_names_[i],
        .elevation = el,
        .azimuth = az,
        .zenith_dist = 90.0 - el,
        .is_rising = false,
    });
  }

  return results;
}

std::vector<SolarBody> AstrometryEngine::CalculateSolarSystem(
    const Observer& obs, std::chrono::system_clock::time_point time) const {
  InitializeNovas();
  std::vector<SolarBody> results;

  return results;
}

}  // namespace engine
