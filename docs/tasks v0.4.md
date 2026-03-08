# 📊 Zenith Finder v0.4: Engineering Task List

## 🎨 1. Advanced TUI (FTXUI Enhancements)

* [x] **Layout Refactoring:**
    *   Swap the primary focus: Move "Zenith Stars" to the main/largest panel.
    *   Move "Solar System" to a secondary panel or bottom tray.
* [x] **Virtualized Scrolling:**
    *   Implement scrolling for the star list using FTXUI `Menu` and `frame` components.
    *   (Note: Virtualization for 5000+ entries still pending optimization in Renderer).
* [x] **Interactive Sorting:**
    *   Add headers with sorting indicators (e.g., ▲/▼).
    *   Implement logic to sort results by Name, Elevation, Azimuth, or Magnitude.
* [x] **Dynamic Filtering UI:**
    *   Add input fields or sliders for Magnitude range (e.g., Mag < 6.0).
    *   Add input fields for Elevation cutoff (e.g., El > 15°).

---

*Note: Remaining tasks for Engine Optimization and Testing have been moved to [v0.5](tasks%20v0.5.md).*

### Suggested Team Allocation (v0.4):

* **Engineer A (TUI Expert):** Virtualized scrolling, sorting logic, and layout refactoring.
