#include "windows_location_provider.hpp"

#include <iostream>

namespace app {

WindowsLocationProvider::WindowsLocationProvider() {
  HRESULT hr = CoCreateInstance(CLSID_Location, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&location_));
  if (SUCCEEDED(hr)) {
    IID reportTypes[] = {IID_ILatLongReport};
    hr = location_->RequestPermissions(nullptr, reportTypes, 1, TRUE);
    if (SUCCEEDED(hr)) {
      initialized_ = true;
    }
  }
}

WindowsLocationProvider::~WindowsLocationProvider() {
  if (location_) {
    location_->Release();
  }
}

engine::Observer WindowsLocationProvider::GetLocation() {
  if (!initialized_) return last_known_obs_;

  ILatLongReport* lat_long_report = nullptr;
  HRESULT hr = location_->GetReport(
      IID_ILatLongReport,
      reinterpret_cast<ILocationReport**>(&lat_long_report));

  if (SUCCEEDED(hr)) {
    DOUBLE latitude = 0.0, longitude = 0.0, altitude = 0.0;
    lat_long_report->GetLatitude(&latitude);
    lat_long_report->GetLongitude(&longitude);
    // Altitude is often not available
    lat_long_report->GetAltitude(&altitude);

    last_known_obs_ = {latitude, longitude, altitude};
    lat_long_report->Release();
  }

  return last_known_obs_;
}

}  // namespace app
