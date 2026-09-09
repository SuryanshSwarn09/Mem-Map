# Mem-Map (C++20 & Qt 6)

A high-performance desktop Disk Space & Storage Analyzer for Windows (inspired by **WinDirStat**, **TreeSize**, and **DaisyDisk**), engineered for speed, low resource usage, and interactive visualization.

---

## Architecture & Features 

```
+---------------------------------------------------------------------------------------------------------+
|                                              Mem-Map (Qt 6)                                             |
|  [Target: C:\ (320 GB free)] [Browse...] [⚡ Scan Now] [✖ Cancel]  [📑 Export Report ▾] [💾 Snapshot ▾]  |
|  [🏠 Root › Users › surya › Downloads]                                                                  |
+---------------------------------------------------------------------------------------------------------+
| QSplitter                                                                                               |
|  +-------------------------------------------------+  +----------------------------------------------+  |
|  | Tabs: [Directory Tree] [Types] [Top 100] [Diff] |  | Visualizer: [▦ Treemap][🔘 Sunburst]         |  |
|  | Name         | Usage% | Size    | Files         |  | Color:      [🎨 File Type / 🌡 File Age]     |  |
|  | > Videos     | [====] | 42.1 GB | 120           |  | - Squarified Treemap Layout (Bruls et al.)   |  |
|  | > Music      | [==  ] | 12.4 GB | 850           |  | - DaisyDisk-Style Multi-Ring Sunburst        |  |
|  | > Documents  | [=   ] |  1.2 GB | 320           |  | - Thermal File Age Heatmap (Hot vs Stale)    |  |
|  | > Code       | [    ] |  0.4 GB | 540           |  | - 3D Cushion / Gradient Shading              |  |
|  |                                                 |  | - Hover Tooltips & Double-Click Drill-Down   |  |
|  +-------------------------------------------------+  +----------------------------------------------+  |
+---------------------------------------------------------------------------------------------------------+
| StatusBar: Scanning: C:\Users\... | Files: 145,210 | Total: 84.2 GB | Time: 04.2s                       |
+---------------------------------------------------------------------------------------------------------+
```

---

## Key Capabilities

### 1. Blazing-Fast Asynchronous Scanner
- **Multithreaded Traversal**: [`ScannerEngine`](src/core/ScannerEngine.h) runs on a dedicated background worker thread (`QThread`), keeping the UI responsive at 60 FPS without freezing.
- **Permission Handling**: Skips inaccessible system folders (e.g. `System Volume Information`) cleanly using `std::filesystem::directory_options::skip_permission_denied` and `try/catch` guards.
- **Loop & Junction Prevention**: Checks Windows attributes for `FILE_ATTRIBUTE_REPARSE_POINT` to prevent infinite recursion cycles caused by NTFS junction points (e.g., `Application Data` → `AppData\Roaming`).
- **Throttled Signals**: Progress updates are throttled to 60ms intervals to prevent flooding Qt's event loop during high-speed scans (>50,000 files/sec).

### 2. Lightweight Memory Tree & Bottom-Up Rollup
- **Cache-Friendly**: [`DiskNode`](src/core/DiskNode.h) provides a minimal memory footprint without heavyweight overhead.
- **Bottom-Up Size Calculation**: Recursive post-order rollup sums all child file sizes into parent directories.
- **Automatic Descending Sort**: Children are sorted by size descending to immediately highlight storage hogs.

### 3. Dual Interactive Visualizers (Treemap & Sunburst)
- **Squarified Treemap View** ([`TreemapWidget`](src/ui/TreemapWidget.h)):
  - Implements the Bruls, Huizing, and van Wijk Squarified Treemap layout algorithm, maintaining aspect ratios close to 1.0 (avoiding thin, unreadable strips).
  - Cushion / 3D gradient shading with subtle dark separators.
  - **Hardware-Accelerated Smooth Zoom Animation**: Utilizes `QVariantAnimation` with `QEasingCurve::OutCubic` over 280ms to smoothly expand or contract tile geometry with dual-buffer pixmap cross-fading, eliminating abrupt visual snapping when drilling down or ascending.
- **DaisyDisk-Style Sunburst View** ([`SunburstWidget`](src/ui/SunburstWidget.h)):
  - Multi-layered concentric annular rings partitioning $[0, 360^\circ]$ based on relative directory size up to 4 levels deep.
  - Interactive center core displaying active folder name, total size, and acting as a click-to-zoom-out button.
- **Visualizer Color Modes**:
  - **File Category Mode** (`[ 🎨 File Type ]`):
    - 🟣 **Video**: MP4, MKV, AVI, MOV (`#9C27B0`)
    - 🔵 **Document**: PDF, DOCX, XLSX, TXT (`#2196F3`)
    - 🟠 **Image**: JPG, PNG, WEBP, SVG (`#FF9800`)
    - 🟡 **Archive**: ZIP, RAR, 7Z, ISO (`#FFC107`)
    - 🟢 **Executable**: EXE, DLL, SYS (`#4CAF50`)
    - 🔴 **Code / Dev**: CPP, PY, JS, HTML (`#E91E63`)
    - 🔷 **Audio**: MP3, WAV, FLAC (`#00BCD4`)
  - **Thermal File Age Heatmap Mode** (`[ 🌡 File Age ]`):
    - 🟥 **< 7 days**: `#F85149` (Hot Coral Red - recently modified active work)
    - 🟨 **7 - 30 days**: `#D29922` (Warm Amber Gold - modified this month)
    - 🟩 **1 - 6 months**: `#3FB950` (Fresh Green - modified within half a year)
    - 🟦 **6 - 12 months**: `#388BFD` (Cool Electric Blue - aging data)
    - ⬜ **1 - 2 years**: `#6E7681` (Muted Steel - dormant data)
    - ⬛ **> 2 years**: `#30363D` (Cold Deep Slate - abandoned files, prime cleanup targets)
    - **Bottom-Up Age Rollup**: Folders recursively reflect the newest modification date of their contents.
    - **Dynamic Legend Bar**: Shows a compact thermal swatch bar directly below the visualizer header.
- **Interactivity**:
  - **Hover**: Rich HTML tooltip showing name, formatted size, relative percentage, file type, exact Last Modified timestamp (`yyyy-MM-dd hh:mm`), relative age (`Today`, `4d ago`, `2mo ago`), and full path.
  - **Click**: Bidirectional synchronization with the Directory TreeView.
  - **Double-Click**: Drill down into any folder.
  - **Right-Click Menu**: Open in Windows File Explorer or copy full path.

### 4. Comprehensive Exporting & Reports
- **Standalone Offline HTML5 Report (`.html`)** ([`ReportExporter`](src/core/ReportExporter.h)):
  - **100% Self-Contained**: Zero external CDN scripts, stylesheets, or internet dependencies; works completely offline.
  - **Embedded Canvas Treemap**: Interactive client-side treemap with hover tooltips, click-to-drill-down, and zoom-out controls.
  - **Summary Dashboard**: KPI cards for Total Space, File Count, Subdirectories, and Top Extension, plus an Extension Breakdown table and a Top 50 Largest Files table.
  - Automatically offers to open in your default browser immediately after export.
- **Spreadsheet Export (`.csv`)**:
  - Exports tabular file data with UTF-8 BOM encoding for native Windows Excel compatibility.
- **Raw Tree Export (`.json`)**:
  - Serializes the entire hierarchical tree structure and scan metrics.

### 5. Disk Snapshot Comparison (Diff View)
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

### 6. Navigation & Analytics Tabs
- **Directory Tree**: QTreeView with custom capacity bars ([`SizeBarDelegate`](src/ui/SizeBarDelegate.h)).
- **Breadcrumb Navigation**: Clickable path segments with **Up** (`▲`) and **Root** (`🏠`) buttons ([`BreadcrumbWidget`](src/ui/BreadcrumbWidget.h)).
- **File Types Breakdown**: Color badges, file counts, and share percentages ([`ExtensionStatsWidget`](src/ui/ExtensionStatsWidget.h)).
- **Top 100 Largest Files**: Ranked table of the biggest space hogs for immediate disk cleanup ([`TopFilesWidget`](src/ui/TopFilesWidget.h)).

### 7. Keyboard Navigation & Windows Shell Polish
- **Global Keyboard Shortcuts**:
  | Shortcut | Action | Description |
  | :--- | :--- | :--- |
  | **`F5`** | **Scan / Refresh** | Re-scans the currently selected drive or directory target. |
  | **`Backspace`** / **`Alt + Up`** | **Navigate Up** | Zooms out to parent folder, matching standard Windows Explorer behavior. |
  | **`Ctrl + O`** | **Browse Folder** | Opens the native directory picker dialog. |
  | **`Ctrl + S`** | **Save Snapshot** | Quickly saves current scan to a portable `.mmap` snapshot. |
  | **`Ctrl + E`** | **Export HTML** | Quickly generates and opens a standalone interactive HTML5 report. |
- **Native Windows PE Icon Resource**:
  - Embedded multi-resolution `.ico` icon (`16x16`, `32x32`, `48x48`, `64x64`, `128x128`, `256x256`) compiled directly into `Mem-Map.exe` via Windows PE resource script ([`resources/app.rc`](resources/app.rc)).
  - Branded display across Windows Explorer, Taskbar, window title bar, and `Alt + Tab` switcher.

---

## Development Evolution: From 0 to Now

The following breakdown details the engineering progression from the initial concept (0) to the current full-featured release:

### Phase 0: Foundations & Toolchain Setup
- **Objective**: Establish a modern, native Windows C++ toolchain for high execution speed and low memory overhead without bulky runtime runtimes.
- **Environment**: Configured MSYS2 UCRT64 with GCC 13.2, CMake 3.28, Ninja 1.11, and Qt 6.6.1 Base (`Core`, `Gui`, `Widgets`).
- **Build Infrastructure**: Engineered [`CMakeLists.txt`](CMakeLists.txt) with C++20 standard requirements, automated Qt MOC/UIC/RCC pipelines, and created [`run.bat`](run.bat) to streamline runtime path injection.

### Phase 1: Core Engine & Initial Desktop Application
- **Hierarchical Memory Tree ([`DiskNode`](src/core/DiskNode.h))**:
  - Designed an n-ary tree node structure storing name, path, cumulative byte size, child pointers, parent link, and categorized file type.
  - Implemented recursive post-order traversal rollup to compute parent folder sizes strictly from the bottom up.
  - Added automatic descending sorting so the largest disk hogs always surface to the top of lists.
- **Asynchronous Scanner ([`ScannerEngine`](src/core/ScannerEngine.h))**:
  - Implemented a multithreaded worker model utilizing `QThread` to ensure UI rendering remains at 60 FPS during intensive disk I/O.
  - Handled locked/protected directories using `std::filesystem::directory_options::skip_permission_denied` with exception fallbacks.
  - Added Windows API detection of `FILE_ATTRIBUTE_REPARSE_POINT` to prevent cyclic infinite loops through NTFS junctions.
  - Throttled progress signals to 60ms intervals to prevent flooding the Qt event loop.
- **Initial UI & Treemap Visualizer ([`MainWindow`](src/ui/MainWindow.h), [`TreemapWidget`](src/ui/TreemapWidget.h))**:
  - Crafted a modern dark theme interface using Qt Style Sheets (QSS) with a collapsible `QSplitter` layout.
  - Implemented the Bruls, Huizing, and van Wijk (2000) squarified treemap layout algorithm in [`TreemapLayout`](src/ui/TreemapLayout.h) to maintain aspect ratios near 1.0.
  - Built custom `QPainter` treemap rendering with 3D cushion gradients, hover tooltips, double-click folder drilling, and context menus for Windows Explorer actions.
  - Integrated [`DiskTreeModel`](src/ui/DiskTreeModel.h) with custom percentage capacity bars ([`SizeBarDelegate`](src/ui/SizeBarDelegate.h)).
  - Added breadcrumb navigation ([`BreadcrumbWidget`](src/ui/BreadcrumbWidget.h)), file extension breakdown ([`ExtensionStatsWidget`](src/ui/ExtensionStatsWidget.h)), and top 100 space hogs ([`TopFilesWidget`](src/ui/TopFilesWidget.h)).
- **Unit Test Foundation ([`test_main.cpp`](tests/test_main.cpp))**:
  - Created automated test suites for bottom-up size calculations, treemap layout geometry, and real filesystem scanning.

### Phase 2: Dual Visualizer & DaisyDisk-Style Sunburst Chart
- **Objective**: Provide an alternative radial visual perspective to complement the rectangular treemap.
- **Sunburst Visualizer ([`SunburstWidget`](src/ui/SunburstWidget.h))**:
  - Developed a multi-layered concentric annular ring chart partitioning $[0, 360^\circ]$ based on relative directory weight up to 4 levels deep.
  - Implemented polar coordinate math ($r, \theta$) for hit-testing mouse interactions, tooltips, and drill-downs.
  - Created an interactive center hub displaying the current root folder name, total size, and serving as a single-click "Zoom Out" control.
- **View Switcher Toolbar**:
  - Added a segmented view toggle (`[ ▦ Treemap ]` vs `[ 🔘 Sunburst ]`) synchronized via `QStackedWidget` with shared breadcrumb state.

### Phase 3: Project Rebranding to "Mem-Map"
- **Objective**: Shift from a generic utility moniker to an identifiable, dedicated brand.
- **Changes**:
  - Renamed the project, CMake compilation targets, binary output (`Mem-Map.exe`), window titles, and repository documentation to **Mem-Map**.
  - Updated all launcher scripts and test runners.

### Phase 4: Exporting, Standalone Reports & Snapshot Diff View
- **Exporting & Reporting Suite ([`ReportExporter`](src/core/ReportExporter.h))**:
  - **Standalone Offline HTML5 Report**: Generates a 100% self-contained, zero-dependency HTML file with an embedded HTML5 `<canvas>` squarified treemap, interactive tooltips, click drill-down, summary KPI cards, extension charts, and top 50 files.
  - **CSV Export**: Generates spreadsheet data prepended with UTF-8 BOM (`\xEF\xBB\xBF`) for seamless loading into Microsoft Excel on Windows.
  - **JSON Export**: Serializes the full hierarchical tree for external data analysis.
- **Disk Snapshot Comparison ([`SnapshotEngine`](src/core/SnapshotEngine.h))**:
  - Added `.mmap` JSON snapshot save/load functionality with scan metadata (timestamp, root path, file count, total size).
  - Engineered a recursive tree diffing algorithm that compares two scan trees and categorizes every node as **Added**, **Deleted**, **Modified**, or **Unchanged**, calculating exact signed byte deltas (`deltaBytes = newSize - oldSize`).
- **Snapshot Diff Dashboard ([`SnapshotDiffWidget`](src/ui/SnapshotDiffWidget.h))**:
  - Introduced a dedicated Diff comparison tab with a prominent KPI Net Change banner (Red `+XX GB` for storage growth, Green `-XX GB` for freed space).
  - Built filter buttons (`[ All Changes ]`, `[ 📈 Growth (+) ]`, `[ 📉 Freed Space (-) ]`, `[ Added ]`, `[ 🗑 Deleted ]`) and real-time text search.
  - Created a color-coded hierarchical diff tree displaying item names, net changes, current sizes, baseline sizes, status flags, and paths.
- **Toolbar Popup Menus**:
  - Integrated "Export Report ▾" and "Snapshot ▾" popup menus directly into the main toolbar.
- **Automated Test Expansion**:
  - Added test suites 4 and 5 in [`test_main.cpp`](tests/test_main.cpp) to validate report exporting and snapshot diff operations in CI/local runs.

### Phase 5: Keyboard Navigation & Native Windows Icon Resource
- **Global Keyboard Navigation**: Added standard navigation shortcuts (`F5`, `Backspace`, `Alt+Up`, `Ctrl+O`, `Ctrl+S`, `Ctrl+E`) wired to core actions with live tooltip shortcut badges.
- **Native Icon & PE Resource**: Designed and generated multi-resolution [`resources/app.ico`](resources/app.ico) (16x16 to 256x256), created [`resources/app.rc`](resources/app.rc), and configured CMake to embed the icon directly into the Windows executable PE header and application window.

### Phase 6: Treemap Smooth Zoom Animation
- **Hardware-Accelerated Fluid Zooming**: Eliminated jarring visual snaps by engineering a dual-buffer scaling transition in [`TreemapWidget`](src/ui/TreemapWidget.h) utilizing `QVariantAnimation` with `QEasingCurve::OutCubic` over 280ms.
- **Sub-Pixel Geometry Interpolation**: Interpolates tile coordinates between source bounds and screen bounds, with synchronized opacity cross-fading.
- **Event Safety & Interrupt Resilience**: Added input guards to ignore mouse events during transitions, with graceful interruption handling on window resize or re-rooting.
- **Unit Test Coverage**: Added `testTreemapAnimationGeometry` in [`tests/test_main.cpp`](tests/test_main.cpp) validating interpolation math across all keyframes.

### Phase 7: Thermal File Age Heatmap Mode & Age Shading
- **Filesystem Modification Time Extraction**: Captured high-precision `fs::last_write_time` in [`ScannerEngine`](src/core/ScannerEngine.h) converted to Unix epoch seconds via C++20 `std::chrono::file_clock::to_sys`.
- **Bottom-Up Last Modified Rollup**: Extended [`DiskNode::calculateBottomUpSizes`](src/core/DiskNode.cpp) to recursively roll up timestamps so directories reflect the most recently touched file inside them.
- **Thermal Visualizer Shading**: Added `ColorMode` (`FileType` vs `FileAge`) to both [`TreemapWidget`](src/ui/TreemapWidget.h) and [`SunburstWidget`](src/ui/SunburstWidget.h), shading visualizer tiles on a 6-tier thermal spectrum (Hot Red `< 7d` to Cold Slate `> 2yr`).
- **Interactive UI Toggle & Legend**: Added a dedicated `[ 🎨 File Type ]` / `[ 🌡 File Age ]` toolbar toggle and an auto-toggling compact color swatch legend bar to [`MainWindow`](src/ui/MainWindow.h).
- **Tooltip Timestamps & Age Calculations**: Enriched hover tooltips with exact modification time and human-friendly relative age indicators (`Today`, `4d ago`, `1mo ago`, `2y ago`).
- **Unit Test Coverage**: Added `testFileAgeHeatmap` in [`tests/test_main.cpp`](tests/test_main.cpp) validating timestamp conversions, relative age strings, directory rollup, and color boundary logic.

---

## 🔄 Continuous Improvement & Parallel Documentation Sync Matrix

To guarantee that code evolution and documentation never drift apart, every improvement to Mem-Map is tracked in this synchronized ledger.

### 1. Parallel Improvement Ledger

| Phase / Milestone | Core Capabilities Added | Key Source Modules | Commit Range | Automated Tests | Docs Sync |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Phase 0: Foundations** | CMake, Ninja, C++20 standard, Qt6 base, `run.bat` launcher. | [`CMakeLists.txt`](CMakeLists.txt), [`run.bat`](run.bat) | `f42f33e` | Toolchain verification | ✅ Synced |
| **Phase 1: Core Engine** | Asynchronous `ScannerEngine`, `DiskNode` tree, post-order size rollup. | [`ScannerEngine.cpp`](src/core/ScannerEngine.cpp), [`DiskNode.cpp`](src/core/DiskNode.cpp) | `f42f33e` | `testDiskNodeBottomUp` | ✅ Synced |
| **Phase 2: UI & Visualizer**| Dark QSS theme, TreeView with `SizeBarDelegate`, Squarified Treemap. | [`MainWindow.cpp`](src/ui/MainWindow.cpp), [`TreemapWidget.cpp`](src/ui/TreemapWidget.cpp) | `f42f33e`, `8acdf71` | `testTreemapLayout` | ✅ Synced |
| **Phase 3: Radial Sunburst**| Multi-ring DaisyDisk radial visualizer, view stack switcher. | [`SunburstWidget.cpp`](src/ui/SunburstWidget.cpp), [`MainWindow.cpp`](src/ui/MainWindow.cpp) | `a422cfe` | Real directory scan test | ✅ Synced |
| **Phase 4: Reports & Diff** | HTML5 report with canvas, CSV, JSON export, binary `.mmap` snapshots, Tree Diff. | [`ReportExporter.cpp`](src/core/ReportExporter.cpp), [`SnapshotEngine.cpp`](src/core/SnapshotEngine.cpp) | `6b875a6` | `testReportExporter`, `testSnapshotEngine` | ✅ Synced |
| **Phase 5: Native Shell** | Windows PE icon embedding, global shortcuts (`F5`, `Backspace`, `Ctrl+O/S/E`).| [`app.rc`](resources/app.rc), [`app.ico`](resources/app.ico), [`MainWindow.cpp`](src/ui/MainWindow.cpp) | `549e822..144722a` | Shortcut dispatch checks | ✅ Synced |
| **Phase 6: Zoom Animation** | Dual-buffer scaling, cubic easing cross-fading, input interaction guards. | [`TreemapWidget.cpp`](src/ui/TreemapWidget.cpp) | `5a5c042..d94f318` | `testTreemapAnimationGeometry` | ✅ Synced |
| **Phase 7: Age Heatmap** | 6-tier thermal coloring, C++20 `last_write_time`, age legend bar. | [`DiskNode.cpp`](src/core/DiskNode.cpp), [`TreemapWidget.cpp`](src/ui/TreemapWidget.cpp), [`SunburstWidget.cpp`](src/ui/SunburstWidget.cpp) | `b65590d..3d7ae7a` | `testFileAgeHeatmap` | ✅ Synced |
| **Phase 8 (Planned)** | Duplicate File Finder (hash comparison), visualizer real-time search filter. | Planned | Next | Next test suite | 🔄 In Queue |

### 2. Parallel Documentation Maintenance Protocol

Whenever changes or improvements are introduced to this codebase, the following **Documentation Maintenance Rules** are strictly enforced:

1. **Simultaneous Documentation Updates**:
   - Every feature or architectural change must update [`README.md`](README.md) and [`docs/GITBOOK_DOCUMENTATION.md`](docs/GITBOOK_DOCUMENTATION.md) simultaneously with the source code.
   - Any modifications to UI flows, shortcuts, or data models must be immediately reflected in Mermaid.js architecture diagrams and ASCII UI mockups.
2. **Atomic Micro-Commit Standard**:
   - Changes must be partitioned into focused, single-purpose micro-commits with declarative messages.
   - Remote pushes to `origin main` only occur after the full series has been executed and validated locally.
3. **Automated Unit Test Companion**:
   - Every new engine or UI feature must include a corresponding automated unit test in [`tests/test_main.cpp`](tests/test_main.cpp).
   - All tests in `test_runner.exe` must pass with zero errors and zero warnings prior to commit.
4. **Ledger Synchronization**:
   - The Improvement Ledger above must be updated with the newly completed phase, commit hash range, modified files, and test verification names.

---

## 📁 Project Structure

```
E:\Mem-scan\
├── CMakeLists.txt              # CMake build configuration (C++20, Qt6)
├── README.md                   # Project documentation
├── run.bat                     # One-click launcher script
├── .gitignore                  # Git ignore rules (build artifacts, binaries)
├── resources\                  # Application assets & Windows PE resources
│   ├── app.ico                 # Multi-resolution application icon (16x16 - 256x256)
│   └── app.rc                  # Windows resource script for PE icon embedding
├── tools\
│   └── generate_icon.py        # Reproducible multi-size icon generator
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
    └── test_main.cpp           # Automated unit test suite (7 test suites)
```

---

## Technology Stack

| Component | Technology | Rationale |
| :--- | :--- | :--- |
| **Language** | **C++20** | Maximum execution speed, zero-overhead abstractions, `std::filesystem`. |
| **GUI Framework** | **Qt 6** (`Core`, `Gui`, `Widgets`) | Native desktop widgets, hardware-accelerated 2D rendering, robust cross-thread signals/slots. |
| **Build System** | **CMake 3.28 + Ninja** | Fast parallel compilation and standard cross-platform build orchestration. |
| **Compiler** | **GCC 13.2 (UCRT64)** | High-optimization C++20 standard library support on Windows. |

---

## Quick Launch & Build

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
