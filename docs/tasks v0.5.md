# 📊 Zenith Finder v0.5: Engineering Task List

## ⚙️ 1. Engine Optimization & Performance

* [ ] **Predicate-Based Fetching:**
    *   Update `AstrometryEngine::CalculateZenithProximity` to accept a `FilterCriteria` struct (Magnitude, Elevation).
    *   Apply filters early in the calculation loop to minimize expensive coordinate transformations.
* [ ] **Visibility Culling:**
    *   Implement a fixed 0° elevation cutoff directly in the engine's horizontal coordinate calculation.
    *   Discard objects below the horizon before they reach the UI results list.
* [ ] **Memory & Loop Optimization:**
    *   Refactor the star catalog processing to handle 50k+ entries with sub-100ms latency.
    *   Use parallel execution (e.g., `std::execution::par`) if needed for high-density catalogs.

## 🧪 2. Testing & Validation

* [ ] **Performance Benchmarking:**
    *   Create a test case for 5000 stars and measure end-to-end calculation time.
    *   Profile the TUI refresh rate with the large catalog active.
* [ ] **Filtering Accuracy:**
    *   Verify that objects below 0° elevation are correctly excluded from the result set.
    *   Validate magnitude filtering against known catalog values.

---

### Suggested Team Allocation:

* **Engineer B (Engine Specialist):** Predicate-based filtering, visibility culling, and loop optimization.
* **Engineer C (Performance/QA):** Benchmarking, large catalog testing, and memory profiling.
