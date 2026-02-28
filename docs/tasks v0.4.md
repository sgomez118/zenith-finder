# 📊 Zenith Finder v0.4: Engineering Task List

## 🎨 1. Advanced TUI (FTXUI Enhancements)

* [ ] **Layout Refactoring:**
    *   Swap the primary focus: Move "Zenith Stars" to the main/largest panel.
    *   Move "Solar System" to a secondary panel or bottom tray.
* [ ] **Virtualized Scrolling:**
    *   Implement efficient rendering for the star list (supporting 50,000+ entries).
    *   Optimize FTXUI `Renderer` to only process/display visible rows to maintain high frame rates.
* [ ] **Interactive Sorting:**
    *   Add headers with sorting indicators (e.g., ▲/▼).
    *   Implement logic to sort results by Name, Elevation, Azimuth, or Magnitude.
* [ ] **Dynamic Filtering UI:**
    *   Add input fields or sliders for Magnitude range (e.g., Mag < 6.0).
    *   Add input fields for Elevation cutoff (e.g., El > 15°).

## ⚙️ 2. Engine Optimization & Performance

* [ ] **Predicate-Based Fetching:**
    *   Update `AstrometryEngine::CalculateZenithProximity` to accept a `FilterCriteria` struct (Magnitude, Elevation).
    *   Apply filters early in the calculation loop to minimize expensive coordinate transformations.
* [ ] **Visibility Culling:**
    *   Implement a fixed 0° elevation cutoff directly in the engine's horizontal coordinate calculation.
    *   Discard objects below the horizon before they reach the UI results list.
* [ ] **Memory & Loop Optimization:**
    *   Refactor the star catalog processing to handle 50k+ entries with sub-100ms latency.
    *   Use parallel execution (e.g., `std::execution::par`) if needed for high-density catalogs.

## 🧪 3. Testing & Validation

* [ ] **Performance Benchmarking:**
    *   Create a test case for 50,000 stars and measure end-to-end calculation time.
    *   Profile the TUI refresh rate with the large catalog active.
* [ ] **Filtering Accuracy:**
    *   Verify that objects below 0° elevation are correctly excluded from the result set.
    *   Validate magnitude filtering against known catalog values.

---

### Suggested Team Allocation:

* **Engineer A (TUI Expert):** Virtualized scrolling, sorting logic, and layout refactoring.
* **Engineer B (Engine Specialist):** Predicate-based filtering, visibility culling, and loop optimization.
* **Engineer C (Performance/QA):** Benchmarking, large catalog testing, and memory profiling.
