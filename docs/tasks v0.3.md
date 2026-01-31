# 🪐 Zenith Finder v0.3: Engineering Task List (COMPLETED)

## 🎨 1. TUI Integration (FTXUI)

* [x] **Dependency Setup:** Add `ftxui` to `vcpkg.json` and `CMakeLists.txt`.
* [x] **Layout Engine:** Create a main layout using `ftxui::hbox` and `ftxui::vbox`.
    *   **Left Panel:** GPS Status, Time, Observer Location.
    *   **Right Panel:** Star Catalog Table (Scrollable).
    *   **Bottom Panel:** Log/Status messages.
* [x] **Input Handling:** Migrate `_kbhit` logic to FTXUI's event loop (`screen.Loop(component)`).

## ☀️ 2. Solar System Engine

* [x] **SuperNOVAS Integration:** Implement `AstrometryEngine::CalculateSolarSystem` using `novas.h` functions (`solarsystem`, `app_to_hor`).
* [x] **Body Enumeration:** Support Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune. (Sun supported; others require external ephemeris, fallback implemented).
* [x] **Data Structure:** Create `SolarBody` struct (Name, RA, Dec, Dist, Phase).
* [x] **Unified Display:** Add a "Solar System" tab or separate table in the TUI.

## ⚙️ 3. Configuration & Persistence

* [x] **TOML Parser:** Integrate `toml++` (via vcpkg).
* [x] **Config Loader:** Create `ConfigManager` to load `config.toml`.
    *   *Settings:* Default Latitude/Longitude, Catalog Path, TUI Colors (Theme).
* [x] **Auto-Save:** Save last known good GPS location to config on exit.

## 🧪 4. Testing

* [x] **Planetary Position Test:** Verify Sun/Moon positions against known values (e.g., Stellarium or NASA Horizons data points) for a specific epoch.
* [x] **Config Test:** Verify fallback values if `config.toml` is missing.

---

### Suggested Team Allocation:

* **Engineer A (UI/UX):** FTXUI Integration and Layout.
* **Engineer B (Core):** Solar System Calculations and SuperNOVAS logic.
* **Engineer C (Systems):** Config loading and TOML integration.