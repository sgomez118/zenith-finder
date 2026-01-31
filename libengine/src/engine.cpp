#include <chrono>
#include <algorithm>
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
    
    for (const auto& star : catalog) {
        cat_entry star_cat;
        make_cat_entry(star.name.data(), "J2000", 0, star.ra / 15.0, star.dec, 0, 0, 0, 0, &star_cat);

        double ra, dec;
        int error = topo_star(jd_tt, delta_t, &star_cat, &site, NOVAS_REDUCED_ACCURACY, &ra, &dec);

        if (error == 0) {
            double zd, az, rar, decr;
            equ2hor(jd_utc, delta_t, NOVAS_REDUCED_ACCURACY, 0.0, 0.0, &site, ra, dec, NOVAS_WEATHER_AT_LOCATION, &zd, &az, &rar, &decr);

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

std::vector<SolarBody> AstrometryEngine::CalculateSolarSystem(const Observer& obs, std::chrono::system_clock::time_point time) {
    // Use built-in low-precision ephemeris for Earth/Sun
    set_planet_provider(earth_sun_calc);
    set_planet_provider_hp(earth_sun_calc_hp);
    
    std::vector<SolarBody> results;
    
    double jd_utc = GetJulianDate(time);
    double delta_t = 69.184; // Approx for 2026
    double jd_tt = jd_utc + delta_t / 86400.0;
    
    on_surface site;
    make_on_surface(obs.latitude, obs.longitude, obs.altitude, 10.0, 1010.0, &site);
    
    struct BodyDef {
        std::string name;
        short id;
    };

    std::vector<BodyDef> bodies = {
        {"Sun", 10}, {"Moon", 11},
        {"Mercury", 1}, {"Venus", 2}, {"Mars", 4},
        {"Jupiter", 5}, {"Saturn", 6}, {"Uranus", 7}, {"Neptune", 8}
    };

    for (const auto& body : bodies) {
        object obj;
        make_object(NOVAS_PLANET, body.id, const_cast<char*>(body.name.c_str()), nullptr, &obj);

        double ra, dec, dis;
        int error = topo_planet(jd_tt, &obj, delta_t, &site, NOVAS_REDUCED_ACCURACY, &ra, &dec, &dis);

        if (error == 0) {
            double zd, az, rar, decr;
            equ2hor(jd_utc, delta_t, NOVAS_REDUCED_ACCURACY, 0.0, 0.0, &site, ra, dec, NOVAS_WEATHER_AT_LOCATION, &zd, &az, &rar, &decr);

            double el = 90.0 - zd;
            if (el > -18.0) { // Keep slightly below horizon for "approaching" visibility (twilight)
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
    }

    // Sort by zenith distance
    std::sort(results.begin(), results.end(), [](const SolarBody& a, const SolarBody& b) {
        return a.zenith_dist < b.zenith_dist;
    });

    return results;
}

} // namespace engine
