# Quality Assurance & Testing

Mem-Map enforces continuous verification through automated regression testing. All critical core modules, layouts, exports, and serialization routines are backed by automated tests.

---

## 1. Running the Test Suite

Execute via PowerShell:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
ninja -C build
./build/test_runner.exe
```

---

## 2. Test Suites Overview

| # | Test Suite | File Verified | Test Objective |
| :--- | :--- | :--- | :--- |
| **1** | `testDiskNodeBottomUp` | `DiskNode.cpp` | Validates post-order hierarchical size rollups and file counters. |
| **2** | `testTreemapLayout` | `TreemapLayout.cpp` | Asserts aspect ratio optimization ($R \approx 1.0$) and geometric tile containment. |
| **3** | `testRealDirectoryScan` | `ScannerEngine.cpp` | Executes a live multithreaded directory scan against project sources. |
| **4** | `testReportExporter` | `ReportExporter.cpp` | Generates and validates HTML, CSV, and JSON report file outputs. |
| **5** | `testSnapshotEngine` | `SnapshotEngine.cpp` | Tests binary `.mmap` round-trip fidelity and signed delta calculations. |
| **6** | `testTreemapAnimationGeometry` | `TreemapWidget.cpp` | Asserts sub-pixel interpolation keyframe geometry at $t=0.0, 0.5, 1.0$. |
| **7** | `testFileAgeHeatmap` | `DiskNode.cpp` | Validates thermal color assignments, age strings, and directory timestamp rollup. |

---

## 3. Sample Verification Output

```text
========================================
      Mem-Map Automated Unit Tests     
========================================
[TEST] Running testDiskNodeBottomUp...
  -> PASSED! Total size: 10000 bytes, files: 3
[TEST] Running testTreemapLayout...
  -> PASSED! Generated 5 valid squarified tiles.
[TEST] Running testRealDirectoryScan on project source tree...
  -> Scan completed in 7ms! Cancelled: no
  -> PASSED! Scanned 29 files, 168.7 KB
[TEST] Running testReportExporter...
  -> PASSED! Successfully exported and verified CSV, JSON, and HTML reports.
[TEST] Running testSnapshotEngine...
  -> PASSED! Snapshot save/load and Tree Diff verified.
[TEST] Running testTreemapAnimationGeometry...
  -> PASSED! Interpolation math validated at all stages (t=0.0, 0.5, 1.0).
[TEST] Running testFileAgeHeatmap...
  -> PASSED! Thermal color mapping, relative age formatting, and timestamp rollup verified.
========================================
  ALL AUTOMATED UNIT TESTS PASSED!      
========================================
```
