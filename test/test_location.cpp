#include <catch2/catch_test_macros.hpp>
#include "../app/location_provider.hpp"
#include "engine.hpp"

using namespace engine;

class MockLocationProvider : public app::LocationProvider {
 public:
  MockLocationProvider(Observer start) : obs_(start) {}
  Observer GetLocation() override {
    // Simulate slight eastward drift (0.001 deg per call)
    obs_.longitude += 0.001;
    return obs_;
  }

 private:
  Observer obs_;
};

TEST_CASE("Mock Location Provider", "[app]") {
  Observer start{0.0, 0.0, 0.0};
  MockLocationProvider mock(start);

  auto obs1 = mock.GetLocation();
  auto obs2 = mock.GetLocation();

  REQUIRE(obs2.longitude > obs1.longitude);
}
