#include "engine.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>

extern "C" {
#include <novas.h>
}

namespace engine {

std::once_flag g_init_flag;
bool g_has_ephemeris = false;

void InitEphemeris() {
  // Fallback to built-in low-precision Earth/Sun
  // SuperNOVAS built-in provider for basic accuracy
  set_planet_provider(earth_sun_calc);
  set_planet_provider_hp(earth_sun_calc_hp);
  g_has_ephemeris = false; // Built-in only supports Earth/Sun well
}

// Helper to convert C++20 chrono time to Julian Date using NOVAS
double GetJulianDate(std::chrono::system_clock::time_point tp) {
  auto sd = std::chrono::floor<std::chrono::days>(tp);
  auto time = tp - sd;

  std::chrono::year_month_day ymd{sd};
  int year = static_cast<int>(ymd.year());
  unsigned month = static_cast<unsigned>(ymd.month());
  unsigned day = static_cast<unsigned>(ymd.day());

  double hour = std::chrono::duration<double, std::ratio<3600>>(time).count();

  return julian_date((short)year, (short)month, (short)day, hour);
}

std::vector<CelestialResult> AstrometryEngine::CalculateZenithProximity(
    const Observer& obs, const std::vector<Star>& catalog,
    std::chrono::system_clock::time_point time) {
  std::call_once(g_init_flag, InitEphemeris);

  std::vector<CelestialResult> results;

  double jd_utc = GetJulianDate(time);

  // For simplicity in v0.1, we'll use delta_t = 69.184 (approximate for
  // 2024-2026)
  double delta_t = 69.184;
  double jd_tt = jd_utc + delta_t / 86400.0;

  on_surface site;
  make_on_surface(obs.latitude, obs.longitude, obs.altitude, 10.0, 1010.0,
                  &site);

  for (const auto& star : catalog) {
    cat_entry star_cat;
    make_cat_entry(star.name.data(), "J2000", 0, star.ra / 15.0, star.dec, 0, 0,
                   0, 0, &star_cat);

    double ra, dec;
    int error = topo_star(jd_tt, delta_t, &star_cat, &site,
                          NOVAS_REDUCED_ACCURACY, &ra, &dec);

    if (error == 0) {
      double zd, az, rar, decr;
      equ2hor(jd_utc, delta_t, NOVAS_REDUCED_ACCURACY, 0.0, 0.0, &site, ra, dec,
              NOVAS_WEATHER_AT_LOCATION, &zd, &az, &rar, &decr);

      double el = 90.0 - zd;
      if (el > 0) {  // Only objects above the horizon
        CelestialResult res;
        res.name = std::string(star.name);
        res.azimuth = az;
        res.elevation = el;
        res.zenith_dist = zd;
        res.is_rising = (az < 180.0);
        results.push_back(res);
      }
    }
  }

  // Sort by zenith distance (ascending)
  std::sort(results.begin(), results.end(),
            [](const CelestialResult& a, const CelestialResult& b) {
              return a.zenith_dist < b.zenith_dist;
            });

  return results;
}

std::vector<SolarBody> AstrometryEngine::CalculateSolarSystem(
    const Observer& obs, std::chrono::system_clock::time_point time) {
  std::call_once(g_init_flag, InitEphemeris);
  
  std::vector<SolarBody> results;

  double jd_utc = GetJulianDate(time);
  double delta_t = 69.184;  // Approx for 2026
  double jd_tt = jd_utc + delta_t / 86400.0;

  on_surface site;
  make_on_surface(obs.latitude, obs.longitude, obs.altitude, 10.0, 1010.0,
                  &site);

  struct BodyDef {
    std::string name;
    short id;
  };

  // Supports full solar system if ephemeris is loaded
  std::vector<BodyDef> bodies = {
      {"Sun", NOVAS_SUN},
      {"Moon", NOVAS_MOON},
      {"Mercury", NOVAS_MERCURY},
      {"Venus", NOVAS_VENUS},
      {"Mars", NOVAS_MARS},
      {"Jupiter", NOVAS_JUPITER},
      {"Saturn", NOVAS_SATURN},
      {"Uranus", NOVAS_URANUS},
      {"Neptune", NOVAS_NEPTUNE},
      {"Pluto", NOVAS_PLUTO}
  };

  for (const auto& body : bodies) {
    // If no ephemeris, skip non-Sun bodies to avoid errors (or check error code)
    if (!g_has_ephemeris && body.name != "Sun") {
        continue;
    }

    object obj;
    // make_object requires casting type
    make_object(NOVAS_PLANET, body.id, const_cast<char*>(body.name.c_str()),
                nullptr, &obj);

    double ra, dec, dis;
    int error = topo_planet(jd_tt, &obj, delta_t, &site, NOVAS_REDUCED_ACCURACY,
                            &ra, &dec, &dis);

    if (error == 0) {
      double zd, az, rar, decr;
      equ2hor(jd_utc, delta_t, NOVAS_REDUCED_ACCURACY, 0.0, 0.0, &site, ra, dec,
              NOVAS_WEATHER_AT_LOCATION, &zd, &az, &rar, &decr);

      double el = 90.0 - zd;
      // Always add if calculated, let UI filter
      SolarBody res;
      res.name = body.name;
      res.azimuth = az;
      res.elevation = el;
      res.zenith_dist = zd;
      res.distance_au = dis;
      res.is_rising = (az < 180.0);
      results.push_back(res);
    }
  }

  // Sort by zenith distance
  std::sort(results.begin(), results.end(),
            [](const SolarBody& a, const SolarBody& b) {
              return a.zenith_dist < b.zenith_dist;
            });

  return results;
}

}  // namespace engine
