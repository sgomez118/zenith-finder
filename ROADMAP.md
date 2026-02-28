# 🗺️ Zenith Finder Project Roadmap

Zenith Finder is a high-performance C++20 utility designed for precise local zenith tracking and celestial orientation using the **Supernovas** astrometry engine.

---

## 🚀 Phase 0.1: The Minimalist CLI (Completed)
**Goal:** Establish the core astrometry engine and time-conversion logic with a zero-dependency CLI.

---

## 🛰️ Phase 0.2: Real-time Tracking & Data Expansion (Completed)
**Goal:** Transition from a snapshot tool to a dynamic tracking utility.

---

## 🪐 Phase 0.3: Solar System & TUI Integration (Completed)
**Goal:** Expand core tracking to the Solar System and upgrade to a robust TUI.

---

## 📊 Phase 0.4: Advanced TUI & Engine Optimization (Next)
**Goal:** Refine the user experience for large datasets and optimize calculation performance.

### Enhanced Interface
* **Layout Swap:** Prioritize the "Zenith Star" view over the "Solar System" view for better primary tracking.
* **Virtualized Scrolling:** Implement high-performance scrolling for the star list to support 50,000+ objects without UI lag.
* **Dynamic Sorting:** Allow users to sort by any column (Elevation, Azimuth, Magnitude).
* **Interactive Filtering:** Add range-based filters (e.g., Magnitude < 6.0, Elevation > 15°).

### Engine Performance
* **Visibility Culling:** Implement a fixed 0° elevation cutoff directly in the calculation loop to filter non-visible objects.
* **Predicate-Based Fetching:** Pass filtering criteria into `CalculateZenithProximity` to avoid unnecessary coordinate transformations for objects below the horizon.
* **Memory Optimization:** Refactor catalog management to handle high-density datasets (50k+ stars) efficiently.

---

## 🌏 Phase 0.5: High-Precision Astrometry
**Goal:** Integrate Earth Orientation Parameters (EOP) for sub-arcsecond accuracy using local data.

* **IERS Bulletin Support:** Implement local file parsers for **IERS Bulletins A, B, C, and D** (`LoadIersBulletin[A-D]`) to support offline use in remote areas.
* **EOP Integration:** Use loaded bulletin data to update Polar Motion ($x, y$) and $DUT1$ parameters in the astrometry frame.
* **Leap Second Management:** Automated handling of UTC-TAI offsets based on current bulletins.

---

## 📡 Phase 0.6: Low Earth Orbit (LEO) Tracking (Deprioritized)
**Goal:** Real-time tracking of satellites and the International Space Station (ISS).
* **TLE Integration:** Support for **Two-Line Element (TLE)** sets via SGP4 algorithms.
* **High-Velocity Logic:** Optimize the refresh loop for high-speed objects.

---

## 🖥️ Phase 0.7: Graphical Interface (Future / Deprioritized)
**Goal:** Migration to a full C++/Qt desktop application.

---

## 🛠️ Technical Standards
* **Language:** C++20 (utilizing `std::format`, `std::numbers`, and `<chrono>`).
* **Standards Compliance:** Strictly follow IAU-76/80 and IAU-2000/2006 models via Supernovas.
* **Performance:** Target sub-100ms execution for all Phase 0.1 calculations.

---

> "Precision is not an accident; it's a choice of standards."