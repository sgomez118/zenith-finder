# 📑 Technical Design Document: Zenith Finder v0.1

## 1. Introduction
**Zenith Finder v0.1** is a minimalist CLI utility designed to identify celestial objects currently at or near the observer's local zenith. This version prioritizes high-performance C++20 implementation and rigorous IAU-compliant astrometry.

## 2. Mathematical Framework

### 2.1 Coordinate Transformation Flow
The program transforms static ICRS (J2000) coordinates into local Horizontal coordinates (Azimuth/Elevation).
1. **ICRS (RA/Dec)**: Input from the static internal catalog.
2. **Terrestrial Transformation**: Accounting for Earth's rotation and orientation at the current system time.
3. **Topocentric (Az/El)**: Final coordinates relative to the observer's specific Latitude/Longitude.



### 2.2 Zenith Proximity Calculation
The "Closeness" to the zenith is calculated as the **Zenith Distance ($Z$)**:
$$Z = 90^\circ - \text{Elevation}$$
* Objects with $Z$ closest to $0^\circ$ are prioritized in the output.

### 2.3 Sky Status Logic (Rising vs. Setting)
We determine the star's status by its position relative to the **Local Meridian**:
* **[RISING]**: The star is in the Eastern Hemisphere ($0^\circ \leq Az < 180^\circ$).
* **[SETTING]**: The star is in the Western Hemisphere ($180^\circ \leq Az < 360^\circ$).



---

## 3. Software Architecture

### 3.1 Timing & Ephemeris (C++20)
We utilize the `<chrono>` library to capture system time and convert it to the formats required by the **Supernovas** engine:
* **UTC**: Captured via `std::chrono::system_clock`.
* **Julian Date (JD)**: Calculated for ephemeris lookup.
* **Terrestrial Time (TT)**: Used for precise planetary and stellar positioning.

### 3.2 Key Components
* **ArgParser**: Handles command-line inputs (`--lat`, `--lon`).
* **StarCatalog**: A `constexpr std::array` storing the J2000 RA/Dec for the 50 brightest stars.
* **AstrometryEngine**: A C++ wrapper for the Supernovas C-API functions (`app_star`, `topo_presys`).
* **TableFormatter**: Uses `std::format` to generate aligned, human-readable CLI tables.

---

## 4. Data Structures

### `Star` (Input)
```cpp
struct Star {
    std::string_view name;
    double ra;  // Degrees
    double dec; // Degrees
};

```

### `CelestialResult` (Output)

```cpp
struct CelestialResult {
    std::string_view name;
    double elevation;
    double azimuth;
    double zenith_dist;
    bool is_rising;
};

```

---

## 5. Execution Logic

1. **Initialization**: Read GPS coordinates from user.
2. **Time Sync**: Capture current `system_clock` time.
3. **Transformation Loop**:
* For each star in the 50-brightest array:
* Run Supernovas transformation.
* Calculate  and Rising/Setting status.
* Store in a results vector.




4. **Sort**: Use `std::sort` to order the results by `zenith_dist` (ascending).
5. **Display**: Print the formatted table to the terminal.

---

## 6. Development Standards

* **Compiler**: Must support C++20 (`-std=c++20`).
* **Library Dependency**: [Supernovas](https://www.google.com/search?q=https://github.com/p-astro/supernovas) (Astrometry engine).
* **Accuracy Target**: Parity with USNO standards within  arcseconds.

```

---

### What's next?
Now that your **Roadmap** and **Design** files are ready, would you like me to write the **C++20 code for the Star Catalog** (the array of the 50 brightest stars with their RA/Dec) so you can drop it directly into your source?
