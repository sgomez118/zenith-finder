#ifndef ZENITH_FINDER_LIBENGINE_INCLUDE_ENGINE_HPP_
#define ZENITH_FINDER_LIBENGINE_INCLUDE_ENGINE_HPP_

#include <calceph.h>

#include <chrono>
#include <memory>
#include <span>
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
  AstrometryEngine();
  ~AstrometryEngine();

  // Pre-builds the NOVAS star objects from the catalog for efficient
  // calculation.
  void SetCatalog(std::span<const Star> catalog);

  // Sets the ephemeris to be used for solar system and high-precision
  // calculations.
  void SetEphemeris(std::shared_ptr<t_calcephbin> ephemeris);

  // Calculates zenith proximity using the pre-built catalog.
  [[nodiscard]] std::vector<CelestialResult> CalculateZenithProximity(
      const Observer& obs, std::chrono::system_clock::time_point time =
                               std::chrono::system_clock::now()) const;

  [[nodiscard]] std::vector<SolarBody> CalculateSolarSystem(
      const Observer& obs, std::chrono::system_clock::time_point time =
                               std::chrono::system_clock::now()) const;

 private:
  // Internal helper to ensure NOVAS is initialized with the current ephemeris.
  void InitializeNovas() const;
  void BuildPlanetsCatalog() const;

  std::vector<std::string> star_names_;

  struct PrebuiltCatalog;
  std::unique_ptr<PrebuiltCatalog> prebuilt_;

  std::shared_ptr<t_calcephbin> ephemeris_;
  mutable int accuracy_ = 0;
  mutable bool initialized_ = false;
};

}  // namespace engine

#endif  // ZENITH_FINDER_LIBENGINE_INCLUDE_ENGINE_HPP_
