#ifndef ZENITH_FINDER_APP_LOCATION_PROVIDER_HPP_
#define ZENITH_FINDER_APP_LOCATION_PROVIDER_HPP_

#include "engine.hpp"

namespace app {

class LocationProvider {
 public:
  virtual ~LocationProvider() = default;
  virtual engine::Observer GetLocation() = 0;
};

class StaticLocationProvider : public LocationProvider {
 public:
  explicit StaticLocationProvider(const engine::Observer& obs) : obs_(obs) {}

  engine::Observer GetLocation() override { return obs_; }

 private:
  engine::Observer obs_;
};

}  // namespace app

#endif  // ZENITH_FINDER_APP_LOCATION_PROVIDER_HPP_
