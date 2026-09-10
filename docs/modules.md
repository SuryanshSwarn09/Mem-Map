# Feature Modules Deep Dive

An in-depth architectural examination of Mem-Map's core subsystems, data structures, algorithms, and design trade-offs.

---

## 1. Module A: Asynchronous Scanner Engine (`ScannerEngine`)
- **Header / Source**: `src/core/ScannerEngine.h`, `src/core/ScannerEngine.cpp`
- **Responsibilities**: High-speed multithreaded directory traversal, NTFS junction avoidance, permission bypass, and throttled UI telemetry.
- **Key Engineering Decisions**:
  - **Worker Thread Isolation**: Inherits from `QThread` to isolate file I/O operations from Qt's GUI rendering thread.
  - **Junction & Loop Detection**: Uses Windows `GetFileAttributesW` to detect `FILE_ATTRIBUTE_REPARSE_POINT`. NTFS junction loops (e.g. `Application Data -> AppData\Roaming`) are skipped, preventing stack exhaustion.
  - **C++20 Time Extraction**: Reads `fs::directory_entry::last_write_time()` and maps it to system clock seconds via `std::chrono::file_clock::to_sys`.
  - **Telemetry Throttling**: Progress signals (`scanProgress`) are time-gated to a minimum 60ms interval using monotonic clocks, preventing event queue flooding.

---

## 2. Module B: Hierarchical Memory Tree (`DiskNode`)
- **Header / Source**: `src/core/DiskNode.h`, `src/core/DiskNode.cpp`
- **Responsibilities**: Cache-friendly storage tree representation, recursive bottom-up metric rollup, age calculations, and descending sort.
- **Key Engineering Decisions**:
  - **Minimal Object Footprint**: Encapsulates path strings, integer byte sizes, timestamp seconds, and a vector of `std::unique_ptr<DiskNode>` children.
  - **Post-Order Bottom-Up Rollup**: Executes `calculateBottomUpSizes()` post-order:
    - Sums cumulative byte size, child file count, and folder count.
    - Concurrently computes `m_lastModifiedTime = std::max(m_lastModifiedTime, child->lastModifiedTime())` so parent directories inherit the modification age of their freshest file.
  - **Relative Age Formatting**: Implements `formatAge()` returning human-readable strings (`Today`, `4d ago`, `1mo ago`, `2y ago`).

---

## 3. Module C: Dual Visualizers & Smooth Zoom Animation
- **Header / Source**: `src/ui/TreemapWidget.h`, `src/ui/SunburstWidget.h`
- **Responsibilities**: Dual-mode spatial disk representations, squarified aspect ratio optimization, polar ring slicing, and fluid zoom transitions.
- **Key Engineering Decisions**:
  - **Squarified Treemap Layout**: Implements Bruls, Huizing, and van Wijk's squarified algorithm in `TreemapLayout.cpp`. Bounded boxes are subdivided along the shortest dimension to produce aspect ratios near 1.0.
  - **DaisyDisk Sunburst Radial Layout**: Maps folder proportions into angular slices ($[0, 360^\circ]$) across 4 concentric annular rings with an interactive click-to-zoom-out center core.
  - **Dual-Buffer Smooth Zooming**:
    - During zoom events, captures `m_prevPixmap` and `m_nextPixmap` views.
    - Drives a 280ms `QVariantAnimation` with `QEasingCurve::OutCubic`, interpolating tile geometries and fading opacity without visual snapping.
    - Suppresses mouse hover and selection events during animation to eliminate race conditions.

---

## 4. Module D: Thermal File Age Heatmap Mode
- **Header / Source**: `src/core/DiskNode.h`, `src/ui/TreemapWidget.cpp`, `src/ui/SunburstWidget.cpp`
- **Responsibilities**: Dynamic tile and ring shading based on modification date to highlight abandoned disk hogs.
- **Thermal Palette Scale**:
  - 🔥 **< 7 days**: `#F85149` (Hot Coral Red) — Active working files.
  - 🟡 **7 - 30 days**: `#D29922` (Warm Amber Gold) — Modified this month.
  - 🟩 **1 - 6 months**: `#3FB950` (Fresh Green) — Standard operational storage.
  - 🔷 **6 - 12 months**: `#388BFD` (Cool Electric Blue) — Aging data.
  - ⬜ **1 - 2 years**: `#6E7681` (Muted Steel) — Stale storage.
  - ❄️ **> 2 years**: `#30363D` (Cold Slate) — Prime candidates for archival/deletion.
- **Visualizer Integration**: Toggled via the `[ 🎨 File Type / 🌡 File Age ]` toolbar button, synchronizing both Treemap and Sunburst views and activating an interactive color swatch legend bar.

---

## 5. Module E: Disk Snapshot & Diff Engine (`SnapshotEngine`)
- **Header / Source**: `src/core/SnapshotEngine.h`, `src/core/SnapshotEngine.cpp`
- **Responsibilities**: Binary `.mmap` serialization, tree diffing, and space-delta forensics.
- **Key Engineering Decisions**:
  - **Binary File Format**: Stores a magic header (`MMAP_SNAP_v1`), creation timestamp, root target path, and a serialized recursive node stream.
  - **Diff Algorithm**: Performs a synchronized traversal between `oldRoot` and `newRoot`. Classifies items into **Added**, **Deleted**, **Modified**, or **Unchanged**, computing `deltaBytes = newSize - oldSize`.
  - **Visual Diff Dashboard**: Integrated in `SnapshotDiffWidget` with growth filters (Red `+GB` vs Green `-GB`).

---

## 6. Module F: Standalone Offline HTML5 Exporter (`ReportExporter`)
- **Header / Source**: `src/core/ReportExporter.h`, `src/core/ReportExporter.cpp`
- **Responsibilities**: Generates zero-dependency interactive reports and data exports.
- **Key Engineering Decisions**:
  - **Interactive Canvas Treemap**: Emits an embedded HTML5 Canvas with client-side JavaScript supporting hover tooltips, click drill-down, and zoom-out.
  - **Air-Gapped Operation**: 100% self-contained without external CDN references, fonts, or scripts.
  - **Tabular Data Support**: Also provides UTF-8 BOM CSV exports for Excel compatibility and raw JSON tree serialization.

---

## 7. Module G: Duplicate File Finder Subsystem (`DuplicateFinder`)
- **Header / Source**: `src/core/DuplicateFinder.h`, `src/core/DuplicateFinder.cpp`, `src/ui/DuplicateFilesWidget.h`, `src/ui/DuplicateFilesWidget.cpp`
- **Responsibilities**: Multi-pass hash-verified duplicate detection, wasted storage accounting, smart auto-selection policies, and safe Recycle Bin purging.
- **Key Engineering Decisions**:
  - **3-Pass Progressive Filter Pipeline**:
    - *Pass 1 (Size Bucketing)*: $O(N)$ partitioning of regular files into exact byte-size buckets (`std::unordered_map<int64_t, std::vector<DiskNode*>>`). Single-file buckets and 0-byte files are pruned with zero disk read I/O.
    - *Pass 2 (Partial 4KB Header Hashing)*: Computes MD5 on the initial 4096-byte chunk, weeding out size collisions before reading large files.
    - *Pass 3 (Chunked Full Content Hashing)*: Reads matching candidates in 64 KB streaming buffers through `QCryptographicHash` (MD5 or SHA-256) with responsive cancellation check points.
  - **Wasted Space Accounting**:
    - For each group of $K$ identical files of size $S$, $(K - 1) \times S$ bytes are quantified as recoverable storage, sorted descending by impact.
  - **Interactive Management UI (`DuplicateFilesWidget`)**:
    - **KPI Metrics Header**: Displays Total Wasted Space (styled in high-contrast Coral Red `#F85149`), Duplicate Groups, Duplicate Files, and Live Selected Cleanup metrics.
    - **Smart Auto-Selection**: Offers 1-click selection heuristics including `Keep Newest` (marks all older copies), `Keep Oldest` (marks all newer copies), and `Select All Duplicates` (retains only the primary file).
    - **Live Search Filtering**: Real-time substring query filtering across filenames and full paths.
    - **Safe Recycle Bin Deletion**: Deletes files using `QFile::moveToTrash()` to ensure deleted duplicates can be recovered from the Windows Recycle Bin if needed.

