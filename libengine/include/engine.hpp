#ifndef LIBENGINE_INCLUDE_ENGINE_HPP_
#define LIBENGINE_INCLUDE_ENGINE_HPP_

#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace engine {

struct Star {
    std::string_view name;
    double ra;  // Right Ascension in degrees
    double dec; // Declination in degrees
};

struct CelestialResult {
    std::string name;
    double elevation;
    double azimuth;
    double zenith_dist;
    bool is_rising;
};

struct Observer {
    double latitude;
    double longitude;
    double altitude;
};

// The 50 brightest stars catalog
constexpr std::array<Star, 50> kStarCatalog = {{
    {"Sirius", 101.287, -16.716},
    {"Canopus", 95.988, -52.696},
    {"Rigil Kentaurus", 219.902, -60.833},
    {"Arcturus", 213.915, 19.182},
    {"Vega", 279.235, 38.784},
    {"Capella", 79.172, 45.998},
    {"Rigel", 78.634, -8.202},
    {"Procyon", 114.825, 5.225},
    {"Achernar", 24.429, -57.237},
    {"Betelgeuse", 88.793, 7.407},
    {"Hadar", 210.951, -60.373},
    {"Altair", 297.696, 8.868},
    {"Acrux", 186.649, -63.099},
    {"Aldebaran", 68.98, 16.509},
    {"Antares", 247.352, -26.432},
    {"Spica", 201.298, -11.161},
    {"Pollux", 116.329, 28.026},
    {"Fomalhaut", 344.413, -29.622},
    {"Deneb", 310.358, 45.28},
    {"Mimosa", 191.93, -59.689},
    {"Regulus", 152.093, 11.967},
    {"Adhara", 104.656, -28.972},
    {"Castor", 113.65, 31.888},
    {"Gacrux", 187.791, -57.113},
    {"Shaula", 263.402, -37.103},
    {"Bellatrix", 81.283, 6.349},
    {"Elnath", 81.573, 28.608},
    {"Miaplacidus", 138.3, -69.717},
    {"Alnilam", 84.053, -1.202},
    {"Alnair", 332.058, -46.961},
    {"Alnitak", 85.19, -1.943},
    {"Alioth", 193.507, 55.959},
    {"Dubhe", 165.93, 61.751},
    {"Mirfak", 51.081, 49.857},
    {"Wezen", 106.381, -26.393},
    {"Kaus Australis", 276.191, -34.385},
    {"Avior", 125.628, -59.513},
    {"Alkaid", 206.885, 49.313},
    {"Sargas", 264.33, -42.998},
    {"Menkalinan", 89.867, 44.947},
    {"Atria", 252.166, -69.028},
    {"Alhena", 99.428, 16.399},
    {"Peacock", 306.412, -56.735},
    {"Alsephina", 137.047, -47.343},
    {"Mirzam", 95.675, -17.956},
    {"Alphard", 141.897, -8.658},
    {"Polaris", 37.946, 89.264},
    {"Hamal", 31.681, 23.462},
    {"Algieba", 154.993, 19.842},
    {"Diphda", 10.897, -17.986}
}};

class AstrometryEngine {
public:
    static std::vector<CelestialResult> CalculateZenithProximity(const Observer& obs, std::chrono::system_clock::time_point time = std::chrono::system_clock::now());
};

} // namespace engine

#endif // LIBENGINE_INCLUDE_ENGINE_HPP_
