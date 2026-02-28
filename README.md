# Zenith Finder: Celestial Tracker

## Overview
Zenith Finder is a high-performance C++20 TUI application designed for precise local zenith tracking and celestial orientation. It identifies stars and solar system bodies currently visible from your location, prioritizing those closest to the local vertical (zenith). It utilizes the **SuperNOVAS** astrometry engine for IAU-compliant coordinate transformations and **FTXUI** for a responsive, dashboard-style terminal interface.

## Core Features
1.  **Zenith Radar**: A real-time 2D polar projection of the sky, highlighting stars and planets directly above you.
2.  **Solar System Tracking**: Accurate positions for the Sun, Moon, and all major planets (Mercury through Neptune).
3.  **Live Dashboard**: A 1Hz refresh loop providing a real-time "live" view of the sky.
4.  **Automatic GPS Integration**: Detects your coordinates automatically via the Windows Location API.
5.  **Dynamic Star Catalog**: Loads external star data from JSON or CSV formats.
6.  **Configurable**: Settings for observer location, refresh rates, and data paths via `config.toml`.
7.  **Data Logging**: Optional logging of celestial results to timestamped CSV files for external analysis.

## Technical Stack
*   **Language**: C++20 (utilizing `<chrono>`, `std::format`, and `<numbers>`)
*   **TUI Framework**: [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
*   **Astronomy Engine**: [SuperNOVAS](https://github.com/supernovas)
*   **Ephemeris Library**: [CALCEPH](https://www.imcce.fr/inpop/calceph) (via SuperNOVAS)
*   **Configuration**: [toml++](https://github.com/marzer/tomlplusplus)
*   **CLI Parser**: [CLI11](https://github.com/CLIUtils/CLI11)
*   **Build System**: CMake

## Setup & Data Files

### 1. Configuration (`config.toml`)
Copy `config.template.toml` to `config.toml` in the project root and customize your default location and data paths:
```toml
[observer]
latitude = 51.5074
longitude = -0.1278
altitude = 0.0

[catalog]
path = 'stars.json'
```

### 2. Star Catalog
The application requires a catalog of stars. You can use the provided `stars.json` or generate a CSV catalog from the HYG database:
```bash
python scripts/fetch_catalog.py
```
This generates `stars.csv` in the root directory. You can configure the path in `config.toml` or via CLI.

### 3. Planetary Ephemeris (Recommended)
For high-precision tracking of all planets, you should provide a JPL binary ephemeris file (e.g., DE442, DE421, or DE405).
1.  Download a binary ephemeris (e.g., [DE442](https://ssd.jpl.nasa.gov/ftp/eph/planets/bsp/)).
2.  Update the `ephemeris.path` in `config.toml` to point to the file.

## Build Instructions

### Prerequisites
*   C++ Compiler (MSVC 2019+ or GCC 10+)
*   CMake 3.20+
*   vcpkg (for dependency management)

### Windows (MSVC)
```powershell
# Configure with "x64-debug" preset (assuming vcpkg is set up)
cmake --preset x64-debug

# Build
cmake --build --preset x64-debug
```

## Usage
Run the executable from the project root.

```powershell
./build/x64-debug/app/zenith-finder.exe [options]
```

### CLI Options:
*   `--lat VALUE`: Manually set observer latitude (degrees).
*   `--lon VALUE`: Manually set observer longitude (degrees).
*   `--alt VALUE`: Manually set observer altitude (meters).
*   `--gps`: Use system GPS location service (overrides manual coordinates).
*   `--catalog PATH`: Path to a custom star catalog file (.json or .csv).
*   `--log`: Enable logging to a timestamped CSV file.

### Key Bindings:
*   `q`: Quit the application.
