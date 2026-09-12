# Continuous Improvement & Documentation Sync

This project enforces parallel synchronization between source code and documentation. Every architectural phase is logged in the synchronized register below.

---

## 1. Parallel Improvement Ledger

| Phase / Milestone | Core Capabilities Added | Key Source Modules | Commit Range | Automated Tests | Docs Sync |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Phase 0: Foundations** | CMake, Ninja, C++20 standard, Qt6 base, `run.bat` launcher. | `CMakeLists.txt`, `run.bat` | `f42f33e` | Toolchain verification | ✅ Synced |
| **Phase 1: Core Engine** | Asynchronous `ScannerEngine`, `DiskNode` tree, post-order size rollup. | `ScannerEngine.cpp`, `DiskNode.cpp` | `f42f33e` | `testDiskNodeBottomUp` | ✅ Synced |
| **Phase 2: UI & Visualizer**| Dark QSS theme, TreeView with `SizeBarDelegate`, Squarified Treemap. | `MainWindow.cpp`, `TreemapWidget.cpp` | `f42f33e`, `8acdf71` | `testTreemapLayout` | ✅ Synced |
| **Phase 3: Radial Sunburst**| Multi-ring DaisyDisk radial visualizer, view stack switcher. | `SunburstWidget.cpp`, `MainWindow.cpp` | `a422cfe` | Real directory scan test | ✅ Synced |
| **Phase 4: Reports & Diff** | HTML5 report with canvas, CSV, JSON export, binary `.mmap` snapshots, Tree Diff. | `ReportExporter.cpp`, `SnapshotEngine.cpp` | `6b875a6` | `testReportExporter`, `testSnapshotEngine` | ✅ Synced |
| **Phase 5: Native Shell** | Windows PE icon embedding, global shortcuts (`F5`, `Backspace`, `Ctrl+O/S/E`).| `app.rc`, `app.ico`, `MainWindow.cpp` | `549e822..144722a` | Shortcut dispatch checks | ✅ Synced |
| **Phase 6: Zoom Animation** | Dual-buffer scaling, cubic easing cross-fading, input interaction guards. | `TreemapWidget.cpp` | `5a5c042..d94f318` | `testTreemapAnimationGeometry` | ✅ Synced |
| **Phase 7: Age Heatmap** | 6-tier thermal coloring, C++20 `last_write_time`, age legend bar. | `DiskNode.cpp`, `TreemapWidget.cpp`, `SunburstWidget.cpp` | `b65590d..3d7ae7a` | `testFileAgeHeatmap` | ✅ Synced |
| **Phase 8: Duplicate Finder** | Multi-pass hash engine (MD5/SHA-256), wasted space tracker, smart auto-select (newest/oldest), safe Recycle Bin cleanup. | `DuplicateFinder.cpp`, `DuplicateFilesWidget.cpp`, `MainWindow.cpp` | `9779951..406d00c` | `testDuplicateFinderAlgorithm` | ✅ Synced |
| **Phase 9: Creator & Socials** | Creator profile modal, social media action links (GitHub, LinkedIn, X, Email), clipboard sharing for Suryansh Swarn. | `CreatorProfile.h`, `AboutDialog.cpp`, `MainWindow.cpp` | `cecbe2c..ca29cbd` | `testCreatorProfile` | ✅ Synced |

---

## 2. Parallel Documentation Maintenance Protocol

Whenever changes or improvements are introduced to this codebase, the following **Documentation Maintenance Rules** are strictly enforced:

1. **Simultaneous Documentation Updates**:
   - Every feature or architectural change must update `README.md` and `docs/GITBOOK_DOCUMENTATION.md` simultaneously with the source code.
   - Any modifications to UI flows, shortcuts, or data models must be immediately reflected in Mermaid.js architecture diagrams and ASCII UI mockups.
2. **Atomic Micro-Commit Standard**:
   - Changes must be partitioned into focused, single-purpose micro-commits with declarative messages.
   - Remote pushes to `origin main` only occur after the full series has been executed and validated locally.
3. **Automated Unit Test Companion**:
   - Every new engine or UI feature must include a corresponding automated unit test in `tests/test_main.cpp`.
   - All tests in `test_runner.exe` must pass with zero errors and zero warnings prior to commit.
4. **Ledger Synchronization**:
   - The Improvement Ledger above must be updated with the newly completed phase, commit hash range, modified files, and test verification names.
