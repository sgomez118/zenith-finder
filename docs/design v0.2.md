# 📑 Technical Design Document: Zenith Finder v0.2

## 1. Overview
**Zenith Finder v0.2** expands the project from a static query tool into a dynamic tracking utility. The primary focus of this release is "Live Tracking"—maintaining an active view of the sky that refreshes in real-time as the Earth rotates and the observer moves.

## 2. New Core Logic: The Dynamic Update Loop

### 2.1 The 1Hz Refresh Cycle
Unlike v0.1, the program will now enter a non-blocking execution loop. 
* **Frequency:** 1Hz (once per second).
* **Process:** The system captures the current sub-second timestamp, recalculates the **Local Sidereal Time (LST)**, and updates the Azimuth/Elevation for the active catalog.

### 2.2 Delta Tracking
To optimize performance, v0.2 introduces "Delta Tracking." 
* **Static Objects (Stars):** Only the transformation from Apparent to Topocentric is recalculated per tick.
* **Moving Objects (Planets/Satellites - Pre-Alpha):** Prepares the pipeline for objects that require orbital propagation.



---

## 3. Software Architecture Expansion

### 3.1 The Observer Singleton / Manager
To support future GPS integration, the observer's location is no longer a static struct passed at launch. A `LocationManager` class will handle:
* **Manual Input:** (Default) Lat/Lon from CLI.
* **Dynamic Input:** Parsing NMEA sentences from a serial port (e.g., `/dev/ttyUSB0`) to update the observer's position in real-time if the user is in a moving vehicle.

### 3.2 Data Provider Interface
v0.2 replaces the hardcoded `std::array` with a `CatalogProvider` interface.
* **Internal Catalog:** The "Top 50" remains the fallback.
* **CSV/JSON Loader:** A new module to parse external files (e.g., Yale Bright Star Catalog). This allows users to track thousands of objects without recompiling the binary.

---

## 4. Feature: Ephemeris File Export
A new requirement for v0.2 is the ability to "Record" a session.
* **Format:** `.csv` or `.txt`.
* **Output:** A time-stamped log of an object’s path relative to the Zenith (Time, Alt, Az, Zenith Dist).
* **Implementation:** Uses `std::ofstream` with a buffered write to ensure minimal impact on the 1Hz calculation loop.

---

## 5. Technical Standards (C++20)

### 5.1 Multi-Threading (Lightweight)
To ensure the CLI remains responsive to user input (like hitting 'Q' to quit) while calculating, we implement:
* **Main Thread:** Handles UI/Table rendering via `std::format`.
* **Worker Thread:** Handles the Supernovas transformation math and serial port GPS polling.

### 5.2 Chrono Advancements
We will utilize `std::chrono::utc_clock` and `std::chrono::tai_clock` for even higher precision when dealing with leap seconds—essential for sub-arcsecond accuracy in v0.2.

---

## 6. Updated CLI Interaction
The interface will now utilize "ANSI Escape Codes" to refresh the table in place, rather than scrolling the terminal.

```bash
$ ./zenith-finder --live --gps /dev/ttyACM0

[LIVE MODE] | Observer: 33.924° N, 118.352° W (GPS Active)
Time: 2026-01-28 21:14:05 UTC | Refresh: 1.0s

NAME            ALTITUDE    AZIMUTH     ZENITH DIST.    STATUS
------------------------------------------------------------------
Capella         82.4°       348.1°      7.6°            [SETTING]
Betelgeuse      59.1°       158.2°      30.9°           [RISING]
...
[Press 'ESC' to Stop | 'S' to Save Ephemeris]

```

---

## 7. Validation Strategy

* **Continuous Integration (CI):** Automate builds using GitHub Actions to ensure C++20 compatibility across GCC and Clang.
* **Drift Testing:** Leave the program running for 24 hours to ensure the "Zenith Dist" matches expected astronomical drift without cumulative timing errors.
