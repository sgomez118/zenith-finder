#include <chrono>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "engine.hpp"

extern "C" {
#include <novas.h>
}

namespace engine {

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

std::vector<CelestialResult> AstrometryEngine::CalculateZenithProximity(const Observer& obs, const std::vector<Star>& catalog, std::chrono::system_clock::time_point time) {
    // Use built-in low-precision ephemeris for Earth/Sun
    set_planet_provider(earth_sun_calc);
    set_planet_provider_hp(earth_sun_calc_hp);
    
    std::vector<CelestialResult> results;
    
    double jd_utc = GetJulianDate(time);
    
    // For simplicity in v0.1, we'll use delta_t = 69.184 (approximate for 2024-2026)
    double delta_t = 69.184;
    double jd_tt = jd_utc + delta_t / 86400.0;
    
    on_surface site;
    make_on_surface(obs.latitude, obs.longitude, obs.altitude, 10.0, 1010.0, &site);
    
    observer novas_obs;
    make_observer_at_site(&site, &novas_obs);

    for (const auto& star : catalog) {
        cat_entry star_cat;
        make_cat_entry(star.name.data(), "J2000", 0, star.ra / 15.0, star.dec, 0, 0, 0, 0, &star_cat);

        sky_pos topo_pos;
        int error = place_star(jd_tt, &star_cat, &novas_obs, delta_t, NOVAS_TOD, NOVAS_REDUCED_ACCURACY, &topo_pos);

        if (error == 0) {
            double zd, az, rar, decr;
            equ2hor(jd_utc, delta_t, NOVAS_REDUCED_ACCURACY, 0.0, 0.0, &site, topo_pos.ra, topo_pos.dec, NOVAS_WEATHER_AT_LOCATION, &zd, &az, &rar, &decr);

            double el = 90.0 - zd;
            if (el > 0) { // Only objects above the horizon
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
    std::sort(results.begin(), results.end(), [](const CelestialResult& a, const CelestialResult& b) {
        return a.zenith_dist < b.zenith_dist;
    });

    return results;
}

} // namespace engine