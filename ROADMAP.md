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

## 🛰️ Phase 0.2: Real-time Tracking & Data Expansion
**Goal:** Transition from a snapshot tool to a dynamic tracking utility.

* **Dynamic Refresh:** Implement a live-updating terminal loop (e.g., 1Hz refresh rate).
* **Extended Catalogs:** Support for external data loading (CSV/JSON) for larger star datasets (e.g., Yale Bright Star Catalog).
* **Hardware Integration:** Parse NMEA sentences from local serial ports for automatic GPS observer positioning.
* **Data Export:** Generate `.txt` or `.csv` ephemeris files for external analysis.

---

## 🖥️ Phase 0.3: Graphical Interface (Qt Integration)
**Goal:** Transform the utility into a full-featured desktop application.

* **Qt Framework:** Migrate from CLI to a C++/Qt-based GUI.
* **Zenith Radar Display:** A circular 2D sky map with the zenith at the center.
* **Visual Filtering:** Interactive tools to filter celestial bodies by magnitude, constellation, or altitude.
* **Location Profiles:** Save and manage multiple observer coordinate profiles.

---

## 🛠️ Technical Standards
* **Language:** C++20 (utilizing `std::format`, `std::numbers`, and `<chrono>`).
* **Standards Compliance:** Strictly follow IAU-76/80 and IAU-2000/2006 models via Supernovas.
* **Performance:** Target sub-100ms execution for all Phase 0.1 calculations.

---

> "Precision is not an accident; it's a choice of standards."