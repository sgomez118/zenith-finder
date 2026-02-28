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

  ILatLongReport* pLatLongReport = nullptr;
  HRESULT hr = location_->GetReport(IID_ILatLongReport,
                                    reinterpret_cast<ILocationReport**>(
                                        &pLatLongReport));

  if (SUCCEEDED(hr)) {
    DOUBLE latitude = 0, longitude = 0, altitude = 0;
    pLatLongReport->GetLatitude(&latitude);
    pLatLongReport->GetLongitude(&longitude);
    // Altitude is often not available
    pLatLongReport->GetAltitude(&altitude);

    last_known_obs_ = {latitude, longitude, altitude};
    pLatLongReport->Release();
  }

  return last_known_obs_;
}

}  // namespace app
