#ifndef APP_WINDOWS_LOCATION_PROVIDER_HPP_
#define APP_WINDOWS_LOCATION_PROVIDER_HPP_

#include "location_provider.hpp"
#include <windows.h>
#include <LocationAPI.h>
#include <sensorsapi.h> // Sometimes needed
#include <atomic>
#include <iostream>
#include <comdef.h>

namespace app {

class WindowsLocationProvider : public LocationProvider {
public:
    WindowsLocationProvider();
    ~WindowsLocationProvider();

    engine::Observer GetLocation() override;

private:
    ILocation* pLocation_ = nullptr;
    engine::Observer last_known_obs_{0.0, 0.0, 0.0};
    bool initialized_ = false;
};

} // namespace app

#endif // APP_WINDOWS_LOCATION_PROVIDER_HPP_
