#include "windows_location_provider.hpp"
#include <iostream>

#pragma comment(lib, "locationapi.lib")

namespace app {

WindowsLocationProvider::WindowsLocationProvider() {
    // Constructor doesn't do much heavy lifting to avoid thread mismatch
    // Initial values
    last_known_obs_ = {51.5074, -0.1278, 0.0}; // Default to London if fails
}

WindowsLocationProvider::~WindowsLocationProvider() {
    if (pLocation_) {
        pLocation_->Release();
        pLocation_ = nullptr;
    }
}

engine::Observer WindowsLocationProvider::GetLocation() {
    if (!initialized_) {
        HRESULT hr = CoCreateInstance(CLSID_Location, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pLocation_));
        if (SUCCEEDED(hr)) {
            // Request permissions if needed (usually handled by OS prompt, but we can request report)
            IID reportTypes[] = { IID_ILatLongReport };
            hr = pLocation_->RequestPermissions(nullptr, reportTypes, 1, TRUE); 
            initialized_ = true;
        } else {
            // std::cerr << "Failed to create ILocation instance: " << std::hex << hr << std::endl;
        }
    }

    if (pLocation_) {
        ILocationReport* pReport = nullptr;
        HRESULT hr = pLocation_->GetReport(IID_ILatLongReport, &pReport);
        if (SUCCEEDED(hr) && pReport) {
            ILatLongReport* pLatLongReport = nullptr;
            hr = pReport->QueryInterface(IID_PPV_ARGS(&pLatLongReport));
            if (SUCCEEDED(hr)) {
                double lat = 0.0;
                double lon = 0.0;
                double alt = 0.0; // Altitude might be in a different report, but let's check LatLong
                
                pLatLongReport->GetLatitude(&lat);
                pLatLongReport->GetLongitude(&lon);
                // pLatLongReport->GetAltitude(&alt); // LatLongReport doesn't always have altitude, checking error
                
                // Update last known
                last_known_obs_.latitude = lat;
                last_known_obs_.longitude = lon;
                
                // Clean up
                pLatLongReport->Release();
            }
            pReport->Release();
        }
    }

    return last_known_obs_;
}

} // namespace app
