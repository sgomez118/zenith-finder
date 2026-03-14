# 🚀 Performance & Memory Optimization Proposals

This document outlines potential optimizations for the Zenith Finder engine and TUI to handle large star catalogs (50k+ entries) with minimal latency and memory overhead.

## 📊 1. Engine Performance Benchmarking (Completed ✅)
Baseline measurements established using `test/benchmarks.exe`.
*   **Synthetic Scaling Results (Debug Build):**
    *   **5k Stars:** ~20ms latency, ~12k allocations.
    *   **10k Stars:** ~30ms latency, ~21k allocations.
    *   **50k Stars:** ~150ms latency, ~105k allocations (~7.4MB heap pressure).
*   **Key Bottlenecks:**
    *   **High Allocation Volume:** ~2 heap allocations per visible star (likely `std::string` copies and `std::vector` growth).
    *   **Rising/Setting Logic:** Calculating a "future frame" (T+1 min) doubles the coordinate transformation overhead for every star in the loop.
    *   **Sorting Overhead:** `std::stable_sort` on the full result set adds significant latency after the math phase.

## ⚡ 2. Parallel Execution (Multi-core) (Completed ✅)
The calculation loop and sorting logic have been parallelized using C++20 execution policies.
*   **Parallel Transformation Loop:** Refactored to use `std::for_each(std::execution::par, ...)` with a contention-free `std::vector<std::optional<CelestialResult>>`.
*   **Thread-Safety Fix:** Identified and resolved thread-safety issues with NOVAS frame objects by using local per-thread copies of frame structs.
*   **Parallel Sorting:** Upgraded `std::stable_sort` to use `std::execution::par`, significantly reducing latency for large result sets.
*   **Performance Gain:** Latency for 50k stars reduced even in Debug mode, with a 28% reduction in heap allocations due to better memory management (`reserve`).

## 🧠 3. Cache-Friendly Data Structures (SoA)
Switching from an Array of Structures (AoS) to a Structure of Arrays (SoA) can dramatically improve CPU cache hit rates.
*   **Contiguous Math Arrays:** Store RA, Dec, and Proper Motion in separate, contiguous `std::vector<double>` arrays. This ensures the CPU pre-fetcher only loads data needed for coordinate math.
*   **Metadata Decoupling:** Move Star names and string IDs into a separate lookup table, indexed by position, so they don't pollute the L1/L2 cache during the visibility culling phase.

## ♻️ 4. Hot-Path Allocation Minimization (Completed ✅)
High-frequency loops (refreshing every 500ms) now perform near-zero heap allocations.
*   **Result Buffering:** Implemented `ResultBuffer` to reuse `std::vector` memory across frames.
*   **Early Predicate Filtering:** Applied `elevation > 0` and name filters *during* the parallel transformation loop.
*   **String Allocation Removal:** Replaced `std::string` with `std::string_view` in result structures and optimized case-insensitive filtering.
*   **Performance Gain:** 50k star calculation allocations reduced from **105,000** to **under 10** per frame.

## 📺 5. TUI Rendering Efficiency (Completed ✅)
*   **Partial DOM Updates:** Implemented lazy string formatting and menu rebuilding. The TUI now uses a dirty-checking mechanism so formatting only occurs when data or filters change.
*   **Debug Overlay:** Added a toggleable debug mode ('d' key) in the sidebar that shows:
    *   `Engine Latency (ms)`
    *   `UI Frame Time (ms)`
    *   `Total Memory Usage`
*   **Performance Gain:** UI Render Time reduced to ~2ms average, with Engine Latency tracking at ~7ms.
