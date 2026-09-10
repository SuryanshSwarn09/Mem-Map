# Mem-Map

> **High-Performance Desktop Disk Space & Storage Analyzer for Windows**  
> Engineered in **C++20** and **Qt 6**, inspired by *WinDirStat*, *TreeSize*, and *DaisyDisk*.

[![Language](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![GUI Framework](https://img.shields.io/badge/GUI-Qt%206.6%2B-green.svg)](https://www.qt.io/)
[![Build System](https://img.shields.io/badge/Build-CMake%20%7C%20Ninja-orange.svg)](https://cmake.org/)
[![Documentation](https://img.shields.io/badge/Docs-GitBook%20Ready-blueviolet.svg)](docs/)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011-0078D6.svg)](https://microsoft.com)

---

## Architecture & Interactive Visualizer

```
+---------------------------------------------------------------------------------------------------------+
|                                              Mem-Map (Qt 6)                                             |
|  [Target: C:\ (320 GB free)] [Browse...] [Scan Now] [Cancel]  [Export Report v] [Snapshot v]            |
|  [Root > Users > surya > Downloads]                                                                     |
+---------------------------------------------------------------------------------------------------------+
| QSplitter                                                                                               |
|  +-------------------------------------------------------------+  +-------------------------------------------+  |
|  | Tabs: [Directory Tree] [Types] [Top 100] [Diff] [Duplicates]  |  | Visualizer: [Treemap] [Sunburst]          |  |
|  | Name         | Usage% | Size    | Files                     |  | Color:      [File Type / File Age]        |  |
|  | > Videos     | [====] | 42.1 GB | 120                       |  | - Squarified Treemap Layout (Bruls et al.)|  |
|  | > Music      | [==  ] | 12.4 GB | 850                       |  | - DaisyDisk-Style Multi-Ring Sunburst     |  |
|  | > Documents  | [=   ] |  1.2 GB | 320                       |  | - Thermal File Age Heatmap (Hot vs Stale) |  |
|  | > Code       | [    ] |  0.4 GB | 540                       |  | - Hardware-Accelerated Smooth Zoom        |  |
|  |                                                             |  | - Hover Tooltips & Double-Click Drill-Down|  |
|  +-------------------------------------------------------------+  +-------------------------------------------+  |
+---------------------------------------------------------------------------------------------------------+
| StatusBar: Scanning: C:\Users\... | Files: 145,210 | Total: 84.2 GB | Time: 04.2s                       |
+---------------------------------------------------------------------------------------------------------+
```

---

## Key Highlights

- **Blazing-Fast Asynchronous Scanner**: Multithreaded `QThread` engine scanning >150,000 files/sec without UI freezing. Handles Windows permissions gracefully and skips NTFS junction loops (`FILE_ATTRIBUTE_REPARSE_POINT`).
- **Squarified Treemap with Smooth Zoom**: High aspect-ratio tile layout with hardware-accelerated dual-buffer cross-fading (`QEasingCurve::OutCubic` over 280ms) for fluid drill-down.
- **DaisyDisk-Style Radial Sunburst**: 360-degree concentric annular rings displaying hierarchical folder sizes up to 4 levels deep, featuring a click-to-ascend center hub.
- **Thermal File Age Heatmap Mode**: 6-tier thermal coloring (Hot Red `< 7d` to Cold Slate `> 2yr`) with recursive directory timestamp rollup to immediately identify abandoned disk hogs.
- **Duplicate File Finder & Safe Cleanup**: Multi-pass hash engine (byte-size grouping, 4KB header check, chunked MD5/SHA-256) calculating exact wasted space, smart auto-selection (Keep Newest/Oldest), and safe Recycle Bin deletion.
- **Disk Snapshot Comparison (Diff View)**: Save portable `.mmap` snapshots to compare any two points in time and instantly pinpoint where disk space disappeared.
- **Zero-Dependency HTML5 Reports**: Export standalone, interactive offline reports with embedded canvas treemaps, CSV spreadsheet data, and raw JSON trees.
- **Native Polish & Shortcuts**: Embedded Windows PE multi-size icon, system tray compatibility, and Explorer-like keyboard shortcuts.

---

## Quick Launch & Build

### Running Pre-Built Binary
Double-click [`run.bat`](run.bat) or execute from PowerShell:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
.\build\Mem-Map.exe
```

### Compiling from Source
Requires **MSYS2 UCRT64** with GCC 13.2+, CMake 3.28+, and Qt 6.6+ Base:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Running Automated Unit Tests
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
.\build\test_runner.exe
```

---

## Keyboard Shortcuts

| Shortcut | Action | Description |
| :--- | :--- | :--- |
| **`F5`** | **Scan / Refresh** | Re-scans the selected directory or drive target. |
| **`Backspace`** / **`Alt + Up`** | **Navigate Up** | Zooms out to the parent folder, matching Windows File Explorer. |
| **`Ctrl + O`** | **Browse Folder** | Opens the native directory picker dialog. |
| **`Ctrl + S`** | **Save Snapshot** | Saves the current scan as a portable `.mmap` snapshot. |
| **`Ctrl + E`** | **Export HTML** | Exports and opens a standalone interactive HTML5 report in your default browser. |

---

## Complete Project Documentation

For in-depth architectural deep dives, Mermaid.js sequence and class diagrams, complete module documentation, and developer setup guides, visit the **[GitBook Documentation Site](docs/)**:

| Chapter | Topic Covered |
| :--- | :--- |
| **[Executive Summary](docs/README.md)** | Project objectives, problems solved, and tech stack specification. |
| **[Architecture & System Design](docs/architecture.md)** | Three-tier architecture, Mermaid flowcharts, threading, and class models. |
| **[Feature Modules Deep Dive](docs/modules.md)** | Low-level analysis of `ScannerEngine`, `DiskNode`, Treemap, Sunburst, and Diff algorithms. |
| **[Changelog & Evolution](docs/changelog.md)** | Narrative breakdown of all major features, performance gains, and bug fixes. |
| **[Developer & Build Guide](docs/developer-guide.md)** | Full MSYS2 UCRT64 toolchain setup, compilation, and standalone distribution packaging. |
| **[Quality Assurance & Testing](docs/quality-assurance.md)** | Automated test suite coverage across all 7 test categories. |
| **[Continuous Improvement Ledger](docs/continuous-improvement.md)** | Live development sync matrix and parallel documentation maintenance protocol. |
| **[Full Single-Page Manual](docs/GITBOOK_DOCUMENTATION.md)** | Complete monolithic manual ready for offline reading and PDF export. |

---

## Continuous Improvement Ledger

| Phase / Milestone | Core Capabilities Added | Key Source Modules | Automated Tests | Docs Sync |
| :--- | :--- | :--- | :--- | :--- |
| **Phase 0: Foundations** | CMake, Ninja, C++20 standard, Qt6 base, `run.bat` launcher. | `CMakeLists.txt`, `run.bat` | Toolchain verification | Synced |
| **Phase 1: Core Engine** | Asynchronous `ScannerEngine`, `DiskNode` tree, post-order size rollup. | `ScannerEngine.cpp`, `DiskNode.cpp` | `testDiskNodeBottomUp` | Synced |
| **Phase 2: UI & Visualizer**| Dark QSS theme, TreeView with `SizeBarDelegate`, Squarified Treemap. | `MainWindow.cpp`, `TreemapWidget.cpp` | `testTreemapLayout` | Synced |
| **Phase 3: Radial Sunburst**| Multi-ring DaisyDisk radial visualizer, view stack switcher. | `SunburstWidget.cpp`, `MainWindow.cpp` | Real directory scan test | Synced |
| **Phase 4: Reports & Diff** | HTML5 report with canvas, CSV, JSON export, binary `.mmap` snapshots, Tree Diff. | `ReportExporter.cpp`, `SnapshotEngine.cpp` | `testReportExporter`, `testSnapshotEngine` | Synced |
| **Phase 5: Native Shell** | Windows PE icon embedding, global shortcuts (`F5`, `Backspace`, `Ctrl+O/S/E`).| `app.rc`, `app.ico`, `MainWindow.cpp` | Shortcut dispatch checks | Synced |
| **Phase 6: Zoom Animation** | Dual-buffer scaling, cubic easing cross-fading, input interaction guards. | `TreemapWidget.cpp` | `testTreemapAnimationGeometry` | Synced |
| **Phase 7: Age Heatmap** | 6-tier thermal coloring, C++20 `last_write_time`, age legend bar. | `DiskNode.cpp`, `TreemapWidget.cpp`, `SunburstWidget.cpp` | `testFileAgeHeatmap` | Synced |
| **Phase 8: Duplicate Finder** | Multi-pass hash engine (MD5/SHA-256), wasted space tracker, smart auto-select (newest/oldest), safe Recycle Bin cleanup. | `DuplicateFinder.cpp`, `DuplicateFilesWidget.cpp`, `MainWindow.cpp` | `testDuplicateFinderAlgorithm` | Synced |

---

## Repository Structure

```text
E:\Mem-scan\
├── CMakeLists.txt              # CMake build configuration (C++20, Qt 6)
├── README.md                   # Project overview & quick start (this file)
├── gitbook-docs.yaml           # GitBook Site Git Sync configuration
├── .gitbook.yaml               # Legacy GitBook directory mapping
├── run.bat                     # Instant PowerShell launch script
├── docs\                       # Modular GitBook documentation chapters
│   ├── README.md               # GitBook home / Executive summary
│   ├── SUMMARY.md              # GitBook left sidebar navigation index
│   ├── architecture.md         # System architecture & Mermaid.js diagrams
│   ├── changelog.md            # Narrative project changelog
│   ├── developer-guide.md      # Toolchain setup & packaging guide
│   ├── modules.md              # Subsystem deep dive & technical design
│   ├── quality-assurance.md    # Automated unit testing suite documentation
│   ├── continuous-improvement.md# Sync ledger & contribution protocol
│   └── GITBOOK_DOCUMENTATION.md# Monolithic single-page documentation manual
├── resources\                  # Application assets & Windows PE resources
├── tools\                      # Reproducible asset generator scripts
├── src\                        # C++20 application source code
│   ├── main.cpp                # Application entry point & High-DPI scaling
│   ├── core\                   # Scanner, DiskNode, Exporter, Snapshot engines
│   └── ui\                     # MainWindow, Treemap, Sunburst, Breadcrumbs
└── tests\                      # Automated unit test suite (7 test suites)
```
