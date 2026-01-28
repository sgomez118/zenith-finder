This task list is designed to take **Zenith Finder v0.1** from concept to a working CLI tool. It is broken down by functional area so that multiple engineers can work in parallel.

---

# ✅ Zenith Finder v0.1: Engineering Task List

## 🏗️ 1. Project Scaffolding & Environment

* [ ] **Setup CMake Build System:** Configure `CMakeLists.txt` to enforce the C++20 standard (`set(CMAKE_CXX_STANDARD 20)`).
* [ ] **Dependency Integration:** Configure the build to link the **Supernovas** C library. Ensure include paths and linker flags are correctly set for cross-platform compatibility.
* [ ] **CLI Argument Parser:** Implement a simple handler for command-line arguments (e.g., `--lat`, `--lon`, `--alt`). Validate that inputs are within realistic geographic ranges.

## 🕒 2. Time & Coordinate Modules

* [ ] **C++20 Time Utilities:** Create a utility class using `<chrono>` to fetch current system time and convert it into **Julian Date (JD)** and **Terrestrial Time (TT)** formats required by Supernovas.
* [ ] **Observer Configuration:** Implement the `Observer` struct to hold latitude, longitude, and elevation. Integrate this with the Supernovas `novas_t` observer structures.

## 🌌 3. Data & Catalog Management

* [ ] **Static Star Catalog:** Implement a `constexpr std::array` containing the 50 brightest stars. Each entry must include the star's name and its J2000 ICRS coordinates (RA/Dec).
* [ ] **Coordinate Storage:** Design the `CelestialResult` data structure to store calculated Azimuth, Elevation, Zenith Distance, and Rising/Setting status for each star.

## 🧪 4. Astrometry Engine (Supernovas Wrapper)

* [ ] **Stellar Transformation Logic:** Implement the core transformation function that maps ICRS coordinates to Apparent Place using `app_star()`.
* [ ] **Topocentric Conversion:** Wrap the Supernovas `topo_presys()` (or equivalent) to convert apparent coordinates to the local Horizontal system (Az/El) based on the observer's location.
* [ ] **Refraction Model:** Implement a standard atmospheric refraction correction (1010hPa, 10°C) within the transformation pipeline.

## 🧮 5. Business Logic & Sorting

* [ ] **Zenith Distance Calculation:** Implement the logic to derive Zenith Distance ().
* [ ] **Rising/Setting Detection:** Implement a boolean check based on Azimuth:
* `true` (Rising) if .
* `false` (Setting) if .


* [ ] **Sorting Algorithm:** Implement a `std::sort` routine to order the results by Elevation (descending), ensuring the stars closest to the zenith appear at the top of the output.

## 🖥️ 6. Output & Formatting

* [ ] **CLI Table Generator:** Build the terminal output using C++20 `std::format`.
* Requirements: Aligned columns, precision to 1 decimal point for degrees, and clear status tags (e.g., `[RISING]`).


* [ ] **Horizon Filter:** Ensure only stars with an Elevation  are passed to the formatter.

## 🔍 7. Validation & QA

* [ ] **Internal Accuracy Test:** Compare output coordinates against a known baseline (e.g., Stellarium or the USNO calculator) for a fixed location and time.
* [ ] **Edge Case Testing:** Verify program behavior for observers at the North/South Poles and the Prime Meridian.

---

### Suggested Parallelization:

* **Engineer A:** Tasks 1 & 6 (Build system and UI formatting).
* **Engineer B:** Tasks 2 & 4 (Time logic and Supernovas integration).
* **Engineer C:** Tasks 3 & 5 (Catalog management and sorting logic).