# Mem-Map (C++20 & Qt 6)

A high-performance desktop Disk Space & Storage Analyzer for Windows (inspired by **WinDirStat**, **TreeSize**, and **DaisyDisk**), engineered for speed, low resource usage, and interactive visualization.

---

## 🏗️ Architecture & Features Overview

```
+---------------------------------------------------------------------------------------------------------+
|                                              Mem-Map (Qt 6)                                             |
|  [Target: C:\ (320 GB free)] [Browse...] [⚡ Scan Now] [✖ Cancel]  [📑 Export Report ▾] [💾 Snapshot ▾]  |
|  [🏠 Root › Users › surya › Downloads]                                                                  |
+---------------------------------------------------------------------------------------------------------+
| QSplitter                                                                                               |
|  +-------------------------------------------------+  +----------------------------------------------+  |
|  | Tabs: [Directory Tree] [Types] [Top 100] [Diff] |  | Visualizer: [ ▦ Treemap ] [ 🔘 Sunburst ]    |  |
|  | Name         | Usage% | Size    | Files         |  |                                              |  |
|  | > Videos     | [====] | 42.1 GB | 120           |  | - Squarified Treemap Layout (Bruls et al.)   |  |
|  | > Music      | [==  ] | 12.4 GB | 850           |  | - DaisyDisk-Style Multi-Ring Sunburst        |  |
|  | > Documents  | [=   ] |  1.2 GB | 320           |  | - 3D Cushion / Gradient Shading              |  |
|  | > Code       | [    ] |  0.4 GB | 540           |  | - Color-Coded by File Category               |  |
|  |                                                 |  | - Hover Tooltips & Double-Click Drill-Down   |  |
|  +-------------------------------------------------+  +----------------------------------------------+  |
+---------------------------------------------------------------------------------------------------------+
| StatusBar: Scanning: C:\Users\... | Files: 145,210 | Total: 84.2 GB | Time: 04.2s                       |
+---------------------------------------------------------------------------------------------------------+
```

---

## 🌟 Key Capabilities

### 1. 🚀 Blazing-Fast Asynchronous Scanner
- **Multithreaded Traversal**: [`ScannerEngine`](src/core/ScannerEngine.h) runs on a dedicated background worker thread (`QThread`), keeping the UI responsive at 60 FPS without freezing.
- **Permission Handling**: Skips inaccessible system folders (e.g. `System Volume Information`) cleanly using `std::filesystem::directory_options::skip_permission_denied` and `try/catch` guards.
- **Loop & Junction Prevention**: Checks Windows attributes for `FILE_ATTRIBUTE_REPARSE_POINT` to prevent infinite recursion cycles caused by NTFS junction points (e.g., `Application Data` → `AppData\Roaming`).
- **Throttled Signals**: Progress updates are throttled to 60ms intervals to prevent flooding Qt's event loop during high-speed scans (>50,000 files/sec).

### 2. 🌲 Lightweight Memory Tree & Bottom-Up Rollup
- **Cache-Friendly**: [`DiskNode`](src/core/DiskNode.h) provides a minimal memory footprint without heavyweight overhead.
- **Bottom-Up Size Calculation**: Recursive post-order rollup sums all child file sizes into parent directories.
- **Automatic Descending Sort**: Children are sorted by size descending to immediately highlight storage hogs.

### 3. 📊 Dual Interactive Visualizers (Treemap & Sunburst)
- **Squarified Treemap View** ([`TreemapWidget`](src/ui/TreemapWidget.h)):
  - Implements the Bruls, Huizing, and van Wijk Squarified Treemap layout algorithm, maintaining aspect ratios close to 1.0 (avoiding thin, unreadable strips).
  - Cushion / 3D gradient shading with subtle dark separators.
- **DaisyDisk-Style Sunburst View** ([`SunburstWidget`](src/ui/SunburstWidget.h)):
  - Multi-layered concentric annular rings partitioning $[0, 360^\circ]$ based on relative directory size up to 4 levels deep.
  - Interactive center core displaying active folder name, total size, and acting as a click-to-zoom-out button.
- **Color-Coded File Categories**:
  - 🟣 **Video**: MP4, MKV, AVI, MOV (`#9C27B0`)
  - 🔵 **Document**: PDF, DOCX, XLSX, TXT (`#2196F3`)
  - 🟠 **Image**: JPG, PNG, WEBP, SVG (`#FF9800`)
  - 🟡 **Archive**: ZIP, RAR, 7Z, ISO (`#FFC107`)
  - 🟢 **Executable**: EXE, DLL, SYS (`#4CAF50`)
  - 🔴 **Code / Dev**: CPP, PY, JS, HTML (`#E91E63`)
  - 🔷 **Audio**: MP3, WAV, FLAC (`#00BCD4`)
- **Interactivity**:
  - **Hover**: Rich HTML tooltip showing name, formatted size, relative percentage, and full path.
  - **Click**: Bidirectional synchronization with the Directory TreeView.
  - **Double-Click**: Drill down into any folder.
  - **Right-Click Menu**: Open in Windows File Explorer or copy full path.

### 4. 📑 Comprehensive Exporting & Reports
- **Standalone Offline HTML5 Report (`.html`)** ([`ReportExporter`](src/core/ReportExporter.h)):
  - **100% Self-Contained**: Zero external CDN scripts, stylesheets, or internet dependencies; works completely offline.
  - **Embedded Canvas Treemap**: Interactive client-side treemap with hover tooltips, click-to-drill-down, and zoom-out controls.
  - **Summary Dashboard**: KPI cards for Total Space, File Count, Subdirectories, and Top Extension, plus an Extension Breakdown table and a Top 50 Largest Files table.
  - Automatically offers to open in your default browser immediately after export.
- **Spreadsheet Export (`.csv`)**:
  - Exports tabular file data with UTF-8 BOM encoding for native Windows Excel compatibility.
- **Raw Tree Export (`.json`)**:
  - Serializes the entire hierarchical tree structure and scan metrics.

### 5. 💾 Disk Snapshot Comparison (Diff View)
- **`.mmap` File Persistence** ([`SnapshotEngine`](src/core/SnapshotEngine.h)):
  - Save any scan as a snapshot file capturing the timestamp, root path, and full tree hierarchy.
- **Tree Diffing Algorithm**:
  - Compare any two scans/snapshots (`oldRoot` vs `newRoot`).
  - Identifies **Added**, **Deleted**, **Modified**, and **Unchanged** files/folders.
  - Computes exact size deltas: `deltaBytes = newSize - oldSize`.
- **Snapshot Diff Tab** ([`SnapshotDiffWidget`](src/ui/SnapshotDiffWidget.h)):
  - **Net Change Indicator**: Displays prominent color-coded change (Red `+XX GB` for storage growth, Green `-XX GB` for freed space).
  - **Filter Controls**: `[ All Changes ]`, `[ 📈 Growth (+) ]`, `[ 📉 Freed Space (-) ]`, `[ ✨ Added ]`, `[ 🗑 Deleted ]`, plus live text search.
  - **Color-Coded Hierarchy**: Tree showing Name, Net Change, Current Size, Baseline Size, Status, and Path.

### 6. 🧭 Navigation & Analytics Tabs
- **Directory Tree**: QTreeView with custom capacity bars ([`SizeBarDelegate`](src/ui/SizeBarDelegate.h)).
- **Breadcrumb Navigation**: Clickable path segments with **Up** (`▲`) and **Root** (`🏠`) buttons ([`BreadcrumbWidget`](src/ui/BreadcrumbWidget.h)).
- **File Types Breakdown**: Color badges, file counts, and share percentages ([`ExtensionStatsWidget`](src/ui/ExtensionStatsWidget.h)).
- **Top 100 Largest Files**: Ranked table of the biggest space hogs for immediate disk cleanup ([`TopFilesWidget`](src/ui/TopFilesWidget.h)).

---

## 📁 Project Structure

```
E:\Mem-scan\
├── CMakeLists.txt              # CMake build configuration (C++20, Qt6)
├── README.md                   # Project documentation
├── run.bat                     # One-click launcher script
├── .gitignore                  # Git ignore rules (build artifacts, binaries)
├── src\
│   ├── main.cpp                # Application entry point & High-DPI scaling
│   ├── core\
│   │   ├── DiskNode.h/.cpp     # Lightweight tree node & bottom-up rollup
│   │   ├── ScannerEngine.h/.cpp# Multithreaded recursive directory scanner
│   │   ├── ReportExporter.h/.cpp # HTML/CSV/JSON report exporter
│   │   └── SnapshotEngine.h/.cpp # .mmap snapshot saver & tree diff algorithm
│   └── ui\
│       ├── MainWindow.h/.cpp   # Main window layout, dark theme QSS & toolbar menus
│       ├── TreemapLayout.h/.cpp# Squarified Treemap layout algorithm
│       ├── TreemapWidget.h/.cpp# Custom QPainter treemap visualizer
│       ├── SunburstWidget.h/.cpp # DaisyDisk radial multi-ring visualizer
│       ├── DiskTreeModel.h/.cpp# QAbstractItemModel wrapping DiskNode tree
│       ├── SizeBarDelegate.h/.cpp # In-table visual capacity bar delegate
│       ├── BreadcrumbWidget.h/.cpp # Clickable path breadcrumb bar
│       ├── ExtensionStatsWidget.h/.cpp # File types breakdown table
│       ├── TopFilesWidget.h/.cpp # Top 100 largest files table
│       └── SnapshotDiffWidget.h/.cpp # Visual snapshot comparison dashboard
└── tests\
    └── test_main.cpp           # Automated unit test suite (5 test suites)
```

---

## 🛠️ Technology Stack

| Component | Technology | Rationale |
| :--- | :--- | :--- |
| **Language** | **C++20** | Maximum execution speed, zero-overhead abstractions, `std::filesystem`. |
| **GUI Framework** | **Qt 6** (`Core`, `Gui`, `Widgets`) | Native desktop widgets, hardware-accelerated 2D rendering, robust cross-thread signals/slots. |
| **Build System** | **CMake 3.28 + Ninja** | Fast parallel compilation and standard cross-platform build orchestration. |
| **Compiler** | **GCC 13.2 (UCRT64)** | High-optimization C++20 standard library support on Windows. |

---

## 🚀 Quick Launch & Build

### Running the Application
Double-click [`run.bat`](run.bat) or run from PowerShell:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
.\build\Mem-Map.exe
```

### Running Automated Tests
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
.\build\test_runner.exe
```

### Rebuilding from Source
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
