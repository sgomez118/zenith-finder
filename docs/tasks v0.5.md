# 📊 Zenith Finder v0.5: Engineering Task List

## ⚙️ 1. Engine Optimization & Performance

* [x] **Predicate-Based Fetching:**
    *   Update `AstrometryEngine::CalculateZenithProximity` to accept a `FilterCriteria` struct (Magnitude, Elevation).
    *   Apply filters early in the calculation loop to minimize expensive coordinate transformations.
* [x] **Visibility Culling:**
    *   Implement a fixed 0° elevation cutoff directly in the engine's horizontal coordinate calculation.
    *   Discard objects below the horizon before they reach the UI results list.
* [ ] **Memory & Loop Optimization:**
    *   Refactor the star catalog processing to handle 50k+ entries with sub-100ms latency.
    *   [x] Implement backend pagination (`offset` and `limit`) to avoid passing massive vectors to the UI.
    *   Use parallel execution (e.g., `std::execution::par`) if needed for high-density catalogs.

## 🖥️ 2. UI Navigation & Windowing

* [x] **Moving Window Scrolling:**
    *   Implement automatic offset adjustment in `ZenithUI` when scrolling past boundaries.
    *   Limit active TUI result list to 100 entries to maintain high refresh rates.
    *   Add `[Windowed]` indicator to UI titles when pagination is active.
* [x] **Enhanced Focus Management:**
    *   Implement `Tab` and `Shift+Tab` for switching focus between "Zenith Stars" and "Solar System".
    *   Constrain list selection within window boundaries to prevent unintended jumps and crashes.

## 🧪 3. Testing & Validation

* [ ] **Performance Benchmarking:**
    *   Create a test case for 5000 stars and measure end-to-end calculation time.
    *   Profile the TUI refresh rate with the large catalog active.
* [x] **Filtering & Pagination Accuracy:**
    *   [x] Verify that objects below 0° elevation are correctly excluded from the result set.
    *   [x] Add unit tests for `star_offset`, `star_limit`, `solar_offset`, and `solar_limit` in the engine.

---

### Suggested Team Allocation:

* **Engineer B (Engine Specialist):** Predicate-based filtering, visibility culling, and loop optimization.
* **Engineer C (Performance/QA):** Benchmarking, large catalog testing, and memory profiling.
