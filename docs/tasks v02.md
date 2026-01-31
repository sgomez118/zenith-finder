Here is the structured task list for **Zenith Finder v0.2**, designed to be distributed among your team.

---

# ✅ Zenith Finder v0.2: Engineering Task List (COMPLETED)

## 🧵 1. Concurrency & Performance

* [x] **Implement the Main Update Loop:** Transition from a "run-once" logic to a 1Hz refresh loop using `std::chrono::steady_clock`.
* [x] **Worker Thread Decoupling:** Move the **Supernovas** calculation logic to a background thread to ensure the CLI stays responsive to user input.
* [x] **Thread-Safe Data Sharing:** Implement a "double-buffer" or atomic pointer system to pass `CelestialResult` vectors from the math thread to the UI thread.

## 📡 2. Dynamic Location (GPS/Serial)

* [x] **Serial Port Listener:** Implement a cross-platform serial reader (using `termios` for Linux or `WinAPI` for Windows) to listen for GPS modules.
* [x] **NMEA Sentence Parser:** Write a parser to extract `$GPGGA` or `$GPRMC` sentences to update the observer's Latitude, Longitude, and Altitude in real-time.
* [x] **Manual Override System:** Ensure the CLI can gracefully fall back to manual coordinates if the GPS signal is lost or the hardware is disconnected.

## 📂 3. External Data & IO

* [x] **CSV/JSON Catalog Loader:** Create a `CatalogLoader` class that replaces the static array. It should parse external files (like the Yale Bright Star Catalog) into the internal `Star` format.
* [x] **Ephemeris Logging System:** Implement the "Save to File" feature.
* *Requirements:* Use `std::ofstream`, timestamped filenames (e.g., `zenith_log_20260128.csv`), and a comma-separated format.


* [x] **Buffer Management:** Ensure file I/O is handled efficiently to prevent "stuttering" in the 1Hz refresh loop.

## 🖥️ 4. Advanced CLI & UI

* [x] **ANSI Escape Sequencing:** Implement terminal "re-drawing" logic. Instead of printing new lines, use `\033[H` (Home) to overwrite the existing table for a "live dashboard" effect.
* [x] **User Input Handler:** Capture real-time keystrokes (e.g., 'Q' to quit, 'S' to start/stop logging) without requiring the user to press 'Enter'.
* [x] **Formatting Upgrades:** Use `std::format` to add dynamic headers that show current GPS status and refresh rate.

## 🧪 5. Testing & Accuracy

* [x] **Coordinate Drift Test:** Verify that the 1Hz update matches the Earth's rotation speed ( per second) without lagging.
* [x] **Mock GPS Source:** Create a "fake" serial stream that simulates a moving observer to test the `LocationManager`'s responsiveness.

---

### Suggested Team Allocation for v0.2:

* **Engineer A (Systems/Hardware):** Tasks in Section 2 (Serial/GPS).
* **Engineer B (Core/Architecture):** Tasks in Section 1 & 3 (Concurrency, File I/O, and Data Loading).
* **Engineer C (UI/Frontend):** Tasks in Section 4 (ANSI Dashboard and Input handling).