#ifndef LIBENGINE_INCLUDE_ENGINE_HPP_
#define LIBENGINE_INCLUDE_ENGINE_HPP_

#include <chrono>
#include <string>
#include <vector>

namespace engine {

struct Star {
  std::string name;        // Name of the star
  std::string catalog;     // Catalog name
  long catalog_id;         // Catalog ID
  double ra;               // Right Ascension in degrees
  double dec;              // Declination in degrees
  char coo_qual;           // Coordinate quality
  double pmra;             // Proper motion in RA (mas/yr)
  double pmdec;            // Proper motion in DEC (mas/yr)
  char pm_qual;            // Proper motion quality
  double parallax;         // Parallax (mas)
  char plx_qual;           // Parallax quality
  double radial_velocity;  // Radial Velocity (km/s)
  char rvz_qual;           // Radial velocity quality
  float flux;              // Magnitude
  float flux_err;          // Flux error
  char flux_qual;          // Flux quality
  std::string ids;         // List of Ids
};

struct CelestialResult {
  std::string name;
  double elevation;
  double azimuth;
  double zenith_dist;
  bool is_rising;
};

struct SolarBody {
  std::string name;
  double elevation;
  double azimuth;
  double zenith_dist;
  double distance_au;  // Distance from observer in AU
  bool is_rising;
};

struct Observer {
  double latitude;
  double longitude;
  double altitude;
};

class AstrometryEngine {
 public:
  static std::vector<CelestialResult> CalculateZenithProximity(
      const Observer& obs, const std::vector<Star>& catalog,
      std::chrono::system_clock::time_point time =
          std::chrono::system_clock::now());
  static std::vector<SolarBody> CalculateSolarSystem(
      const Observer& obs, std::chrono::system_clock::time_point time =
                               std::chrono::system_clock::now());
};

}  // namespace engine

#endif  // LIBENGINE_INCLUDE_ENGINE_HPP_
