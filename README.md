# Disk Space Analyzer (C++20 & Qt 6)

A high-performance desktop Disk Space Analyzer for Windows (similar to WinDirStat, TreeSize, and DaisyDisk) engineered for speed, low resource usage, and interactive visualization.

---

## Architecture Overview

```
+---------------------------------------------------------------------------------+
|                                 MainWindow (Qt 6)                               |
|  [Target: C:\ (320 GB free)] [Browse Folder...] [⚡ Scan Now] [✖ Cancel]       |
|  [🏠 Root › Users › surya › Downloads]                                          |
+---------------------------------------------------------------------------------+
| QSplitter                                                                       |
|  +---------------------------------------+  +--------------------------------+  |
|  | Tabs: [Directory Tree] [Types] [Top]  |  | TreemapWidget                  |  |
|  | Name       | Usage% | Size   | Files  |  | - Squarified Treemap layout    |  |
|  | > Videos   | [====] | 42.1GB | 120    |  | - Cushion gradient shading     |  |
|  | > Music    | [==  ] | 12.4GB | 850    |  | - Color-coded by category      |  |
|  | > Docs     | [=   ] |  1.2GB | 320    |  | - Hover tooltips & drill-down  |  |
|  +---------------------------------------+  +--------------------------------+  |
+---------------------------------------------------------------------------------+
| StatusBar: Scanning: C:\Users\... | Files: 145,210 | Total: 84.2 GB | Time: 04.2s|
+---------------------------------------------------------------------------------+
```

### 1. File Scanner (The Backend)
- **Engine**: [`ScannerEngine`](file:///e:/Mem-scan/src/core/ScannerEngine.h) runs on an asynchronous worker thread (`QThread`).
- **Traversal**: Uses C++17 `std::filesystem::directory_iterator` with `skip_permission_denied`.
- **Reparse Points & Symlink Safety**: Win32 `FILE_ATTRIBUTE_REPARSE_POINT` checks prevent infinite directory loops and junction cycles (e.g. `Application Data`).
- **Throttling**: Progress updates are throttled to 60ms to prevent flooding Qt's event loop and ensure smooth 60 FPS UI performance during high-speed scanning (>50,000 files/sec).

### 2. Memory Tree Structure
- **Node**: [`DiskNode`](file:///e:/Mem-scan/src/core/DiskNode.h) is a lightweight, cache-friendly data structure storing:
  - File/folder name, full path, directory flag, file extension.
  - Subtree byte size, file count, and folder count.
  - Pointer to parent, list of child unique pointers (`std::vector<std::unique_ptr<DiskNode>>`).

### 3. Bottom-Up Calculation
- **Aggregation**: Recursive post-order traversal (`calculateBottomUpSizes`) rolls up leaf sizes to their parent folders.
- **Sorting**: Children are sorted descending by size (`sortChildrenBySize`) for instant identification of the largest storage consumers.

### 4. Interactive Squarified Treemap
- **Algorithm**: [`TreemapLayout`](file:///e:/Mem-scan/src/ui/TreemapLayout.h) implements the Bruls, Huizing, and van Wijk **Squarified Treemap Algorithm**, preserving aspect ratios close to 1.0 (avoiding thin strips).
- **Tactile 3D Aesthetics**: [`TreemapWidget`](file:///e:/Mem-scan/src/ui/TreemapWidget.h) renders tiles with cushion shading, category color coding, and subtle dark separators.
- **Interactivity**:
  - **Hover**: Rich HTML tooltip with exact size, percentage of root, category, and file path.
  - **Click**: Highlights the corresponding row in the Directory TreeView.
  - **Double-Click**: Zooms / drills down into that folder.
  - **Right-Click**: Context menu to open the file/folder in Windows Explorer, copy the path, or zoom in.

### 5. Supplementary Analytics
- **Breadcrumb Navigation**: [`BreadcrumbWidget`](file:///e:/Mem-scan/src/ui/BreadcrumbWidget.h) displays clickable folder path segments with "Up" and "Root" buttons.
- **File Types Summary**: [`ExtensionStatsWidget`](file:///e:/Mem-scan/src/ui/ExtensionStatsWidget.h) shows total bytes, share percentage, and file counts aggregated by extension.
- **Top 100 Files**: [`TopFilesWidget`](file:///e:/Mem-scan/src/ui/TopFilesWidget.h) identifies the 100 biggest individual space hogs for immediate cleanup.

---

## Quick Launch & Build

### Running the Application
Double-click [`run.bat`](file:///e:/Mem-scan/run.bat) or run from PowerShell:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
.\build\DiskSpaceAnalyzer.exe
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
