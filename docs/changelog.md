# Changelog & Project Evolution

A narrative breakdown of all architectural milestones, feature additions, and performance enhancements across Mem-Map's development lifecycle.

---

## 1. Major Feature Additions

- **DaisyDisk-Style Radial Sunburst Visualizer**: Introduced multi-layered concentric annular rings partitioning $360^\circ$ by folder proportions, with an interactive center hub for upward navigation.
- **Standalone Offline HTML5 Report Generator**: Self-contained single-file HTML export containing zero external CDN dependencies, embedding an interactive HTML5 Canvas treemap with drill-down capabilities.
- **Snapshot Persistence & Visual Diff View**: Added proprietary binary `.mmap` serialization and a visual diff comparison widget featuring color-coded growth (Red `+GB`) and freed space (Green `-GB`).
- **Hardware-Accelerated Smooth Zoom Animation**: Integrated `QVariantAnimation` with `QEasingCurve::OutCubic` over 280ms, replacing visual tile snapping with dual-buffer pixmap cross-fading and sub-pixel coordinate interpolation.
- **Thermal File Age Heatmap Mode**: Added dynamic visualizer tile and ring shading across a 6-tier thermal spectrum (Hot Red `< 7d` to Cold Slate `> 2yr`), bottom-up recursive directory age rollup, and a toggleable color legend bar.
- **Native Windows PE Resource & Global Keyboard Navigation**: Embedded multi-resolution `.ico` binaries into the executable PE header and mapped standard keyboard shortcuts (`F5`, `Backspace`, `Alt+Up`, `Ctrl+O`, `Ctrl+S`, `Ctrl+E`).

---

## 2. Performance & System Improvements

- **Asynchronous Worker Threading**: Refactored the core traversal engine to execute inside a dedicated `QThread`, isolating disk I/O latency from the main Qt event loop.
- **Monotonic Signal Throttling**: Implemented a 60ms time-gate on worker progress signals, eliminating Qt event queue flooding during high-speed scans.
- **C++20 File Clock Translation**: Leveraged `std::chrono::file_clock::to_sys` for zero-cost conversion of native Win32 filesystem timestamps to Unix seconds.
- **Dual-Buffer Vector Graphics Rendering**: Designed double-buffered off-screen rendering pipelines in both `TreemapWidget` and `SunburstWidget` to maintain 60 FPS drawing speeds during resizing and animation sequences.

---

## 3. Bug Fixes & Architectural Hardening

- **NTFS Infinite Loop Prevention**: Added Win32 attribute checking (`FILE_ATTRIBUTE_REPARSE_POINT`) to skip directory junctions (e.g., `Application Data` → `AppData\Roaming`), eliminating infinite recursion cycles.
- **Permission Denial Bypassing**: Configured `fs::directory_options::skip_permission_denied` and structured exception handling to bypass protected Windows directories (`System Volume Information`, `Recovery`) without aborting active scans.
- **Animation Event Suppression**: Implemented input-guarding during zoom transitions to prevent mouse hover, selection, and double-click race conditions from corrupting view state.
- **Interruption Resilience**: Added graceful animation termination hooks on window resize events and breadcrumb navigation jumps, discarding stale pixmaps and preventing memory leaks.
