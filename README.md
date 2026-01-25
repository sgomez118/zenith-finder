# Zenith Finder: Celestial Tracker

## Overview
Zenith Finder is a C++ desktop application built with Qt that allows users to identify and track celestial bodies visible from their location. It utilizes the **SuperNOVAS** library for high-precision astronomical calculations.

## Core Features
1.  **Visibility Calculation**: Displays a list of celestial bodies visible at a specific time and GPS location.
2.  **Smart Sorting**: Objects are sorted by **brightness** (magnitude) to highlight the most prominent stars/planets.
3.  **Tracking & Ephemeris**: Users can select a specific star to track over a defined time range.
4.  **Ephemeris Export**: Generates an ephemeris file (tracking data) for the selected object and time window.

## Technical Stack
*   **Language**: C++
*   **GUI Framework**: Qt (Widgets or Quick)
*   **Astronomy Engine**: [SuperNOVAS](https://github.com/supernovas) (SNO)
*   **Build System**: CMake

## Architecture Design

### 1. The Engine (Backend)
Wraps the SuperNOVAS C API to provide C++ friendly classes.
*   `CelestialCalculator`: Handles the core visibility logic.
    *   Input: `ObserverLocation` (Lat, Lon, Alt), `DateTime`.
    *   Process: Iterates through catalog of stars/planets, calculates Azimuth/Altitude using SuperNOVAS.
    *   Output: `std::vector<CelestialBody>` sorted by magnitude.
*   `EphemerisGenerator`: Handles the "tracking" use case.
    *   Input: `CelestialBody` ID, `StartTime`, `EndTime`, `Interval`.
    *   Output: Writes a file (e.g., `star_track.txt`) containing position data over time.

### 2. The Interface (Frontend)
*   **Main Window**:
    *   Inputs: Latitude, Longitude, Date/Time picker.
    *   "Scan Sky" button.
*   **Results Table**:
    *   Columns: Name, Type (Star/Planet), Brightness (Mag), Azimuth, Altitude.
    *   Clicking a row opens the "Track Object" dialog.
*   **Track Object Dialog**:
    *   Select Time Duration (e.g., "Next 4 hours").
    *   "Generate Ephemeris" button to save the file.

## Getting Started

### Prerequisites
*   C++ Compiler (GCC/Clang/MSVC)
*   CMake
*   vcpkg

### Build Instructions
```bash
# Clone vcpkg if not already done
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# Configure with CMake (assuming vcpkg is in a standard location or using CMAKE_TOOLCHAIN_FILE)
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```
