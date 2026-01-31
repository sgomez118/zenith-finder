# 🗺️ Zenith Finder Project Roadmap

Zenith Finder is a high-performance C++20 utility designed for precise local zenith tracking and celestial orientation using the **Supernovas** astrometry engine.

---

## 🚀 Phase 0.1: The Minimalist CLI (Current)
**Goal:** Establish the core astrometry engine and time-conversion logic with a zero-dependency CLI.

### Core Engine
* **Astrometry:** Integrate the **Supernovas** C library for IAU-compliant coordinate transformations.
* **Time Handling:** Implement C++20 `<chrono>` for high-precision system time to Julian Date (JD) and Terrestrial Time (TT) conversion.
* **Coordinate Math:** Native C++20 implementation for converting Geodetic coordinates to Topocentric systems.

### Star Catalog & Logic
* **Static Catalog:** A `constexpr` array containing the **50 brightest stars** (J2000 coordinates).
* **Zenith Calculation:** Calculate proximity to the vertical ($90^\circ - \text{Elevation}$).
* **Sky Status Indicators:**
    * **[RISING]**: Object is in the Eastern sky (Azimuth $0^\circ \leq Az < 180^\circ$).
    * **[SETTING]**: Object is in the Western sky (Azimuth $180^\circ \leq Az < 360^\circ$).

### Interface
* Standard CLI input for Latitude and Longitude.
* A formatted, sorted table output highlighting stars closest to the Zenith.

---

## 🛰️ Phase 0.2: Real-time Tracking & Data Expansion (Completed)
**Goal:** Transition from a snapshot tool to a dynamic tracking utility.

* **Dynamic Refresh:** Implement a live-updating terminal loop (e.g., 1Hz refresh rate).
* **Extended Catalogs:** Support for external data loading (CSV/JSON) for larger star datasets (e.g., Yale Bright Star Catalog).
* **Hardware Integration:** Parse NMEA sentences from local serial ports for automatic GPS observer positioning.
* **Data Export:** Generate `.txt` or `.csv` ephemeris files for external analysis.

---

## 🪐 Phase 0.3: Solar System & TUI Integration (Planned)
**Goal:** Expand core tracking to the Solar System and upgrade to a robust TUI.

* **TUI Framework:** Integrate **FTXUI** for a responsive, dashboard-style terminal interface.
* **Solar System:** Implement tracking for the **Sun, Moon, and Planets** (Mercury through Neptune).
* **Interactive Tables:** Scrollable lists for Stars and Solar System bodies.
* **Configuration:** Load default observer location and settings from `config.toml`.

---

## 🖥️ Phase 0.4: Advanced Features & GUI (Future)
**Goal:** Graphical Interface and Advanced Filtering.

* **Qt Framework:** Migrate to a full C++/Qt desktop application.
* **Visual Filtering:** Tools to filter by constellation or magnitude.
* **Zenith Radar:** 2D visualization of the sky.

---

## 📡 Phase 0.5: Low Earth Orbit (LEO) Tracking
**Goal:** Real-time tracking of satellites and the International Space Station (ISS).
* **TLE Integration:** Support for **Two-Line Element (TLE)** sets via SGP4 (Simplified General Perturbations) algorithms.
* **High-Velocity Logic:** Optimize the refresh loop for high-speed objects that cross the zenith in minutes rather than hours.
* **Pass Prediction:** Implement a "Next Pass" calculator to notify users when a specific satellite will reach its local zenith.

---

## 🛠️ Technical Standards
* **Language:** C++20 (utilizing `std::format`, `std::numbers`, and `<chrono>`).
* **Standards Compliance:** Strictly follow IAU-76/80 and IAU-2000/2006 models via Supernovas.
* **Performance:** Target sub-100ms execution for all Phase 0.1 calculations.

---

> "Precision is not an accident; it's a choice of standards."