# Project Overview & Executive Summary

A high-performance, asynchronous disk space and storage analyzer for Windows engineered in C++20 and Qt 6.

---

## 1. Executive Summary
**Mem-Map** is a native Windows desktop systems utility engineered for rapid disk storage analysis, visual space forensics, and interactive disk cleanup. Inspired by the diagnostic depth of **WinDirStat**, the tabular rigor of **TreeSize**, and the fluid radial visual aesthetics of **DaisyDisk**, Mem-Map delivers an ultra-fast, zero-overhead storage inspector that remains completely responsive even under extreme filesystem loads (>150,000 files/second).

---

## 2. Problems Solved
Traditional disk space analyzers frequently suffer from one or more systemic architectural shortcomings:
- **UI Freezes and Thread Starvation**: Blocking GUI threads during synchronous traversal of deep, permission-restricted directory trees.
- **Excessive Memory Footprint**: Inefficient object overhead per scanned file, causing hundreds of megabytes of RAM consumption on large multi-terabyte drives.
- **Static Visualizations**: Lack of fluid animations or interactive drill-down capabilities, causing disorientation during navigation.
- **Inability to Track Disappearing Space**: Users cannot easily identify what caused a sudden drop of 30 GB without manually comparing dated file listings.
- **Lack of Temporal Awareness**: File sizes alone do not reveal whether a 40 GB archive is actively used or has sat abandoned for three years.
- **Reporting Lock-In**: Export formats are typically constrained to static CSV files without interactive visual representations for external stakeholders.

---

## 3. Key Differentiators & Solutions
Mem-Map solves these challenges through:
1. **Asynchronous Multi-Threaded Engine**: An isolated POSIX/Win32-compliant filesystem worker running on a dedicated worker thread with signal throttling.
2. **Compact Cache-Friendly Object Tree**: A lightweight hierarchical node structure (`DiskNode`) with bottom-up post-order rollup.
3. **Dual Reactive Visualizers**:
   - **Squarified Treemap** with hardware-accelerated dual-buffer smooth zoom animations (`QEasingCurve::OutCubic`).
   - **Multi-Ring Sunburst** (DaisyDisk-style) providing an intuitive 360° bird's-eye perspective.
4. **Thermal File Age Heatmap Mode**: A 6-tier thermal color grading scheme from Hot Red (`< 7 days`) to Cold Deep Slate (`> 2 years`) to expose abandoned files instantly.
5. **Snapshot Comparison & Tree Diffing**: Binary `.mmap` state persistence with tree diffing to detect Added, Deleted, and Modified file sizes.
6. **Zero-Dependency HTML5 Report Exporter**: Standalone, interactive client-side reports with embedded HTML5 Canvas treemaps requiring no external web connections.

---

## 4. Technology Stack Specification
| Layer | Technology | Specification / Version | Architectural Rationale |
| :--- | :--- | :--- | :--- |
| **Language** | C++ | C++20 Standard (`-std=c++20`) | Zero-overhead abstractions, `std::filesystem`, `std::chrono::file_clock::to_sys`, memory safety. |
| **GUI Framework** | Qt 6 | Qt 6.6+ Base (`Core`, `Gui`, `Widgets`) | Native desktop widgets, hardware-accelerated 2D raster painting, thread-safe signal/slot IPC. |
| **Compiler** | GCC (MinGW-w64) | GCC 13.2.0 (MSYS2 UCRT64) | Optimal Windows UCRT integration, modern C++20 library implementations. |
| **Build System** | CMake + Ninja | CMake 3.28+, Ninja 1.11+ | Parallel compilation graphs, automated Qt MOC/UIC/RCC code-generation workflows. |
| **Platform Target**| Windows Win32 / PE | Windows 10 & 11 (x86_64) | Embedded multi-resolution PE icon resource, Win32 junction/reparse point detection. |
