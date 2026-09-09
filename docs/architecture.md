# Architecture & System Design

Mem-Map is engineered around a clean three-tier separation of concerns: **Core Engine**, **Presentation / Visualizers**, and **Persistence / Reporting**.

---

## 1. High-Level Component Architecture

```mermaid
graph TD
    subgraph UI_Layer ["Presentation & Visualization Layer"]
        MW["MainWindow (Coordinator)"]
        TV["QTreeView & DiskTreeModel"]
        TM["TreemapWidget (Squarified + Zoom Anim)"]
        SB["SunburstWidget (Radial Multi-Ring)"]
        BC["BreadcrumbWidget (Path Bar)"]
        TF["TopFilesWidget (Top 100 List)"]
        ES["ExtensionStatsWidget (File Types)"]
        SD["SnapshotDiffWidget (Visual Diff)"]
        LEG["HeatmapLegend (Thermal Swatch Bar)"]
    end

    subgraph Core_Layer ["Core Engine Layer"]
        SE["ScannerEngine (QThread Worker)"]
        DN["DiskNode (Hierarchical Memory Tree)"]
        TL["TreemapLayout (Squarified Algorithm)"]
    end

    subgraph Persistence_Layer ["Persistence & Export Layer"]
        RE["ReportExporter (HTML5 Canvas / CSV / JSON)"]
        SN["SnapshotEngine (Binary Serialization & Diff)"]
    end

    subgraph OS_Layer ["Operating System Layer"]
        FS["Windows Filesystem (NTFS / ReFS / FAT32)"]
    end

    %% Interactions
    MW -->|Triggers Scan| SE
    SE -->|Reads metadata & junction tags| FS
    SE -->|Constructs & aggregates| DN
    SE -->|Emits progress throttled| MW
    SE -->|Yields completed tree| MW

    MW -->|Populates| TV
    MW -->|Binds root node| TM
    MW -->|Binds root node| SB
    MW -->|Syncs breadcrumbs| BC
    MW -->|Populates| TF
    MW -->|Populates| ES
    MW -->|Toggles ColorMode| LEG

    TM -->|Computes layout geometry| TL
    DN -->|Supplies thermal & category colors| TM
    DN -->|Supplies thermal & category colors| SB

    MW -->|Exports active tree| RE
    MW -->|Saves / Loads .mmap| SN
    SN -->|Compares two trees| SD
```

---

## 2. Threading & Asynchronous Scan Data Flow

To eliminate UI freezes and starvation, filesystem traversal runs on a dedicated worker thread, dispatching throttled telemetry back to the GUI thread at 60ms intervals.

```mermaid
sequenceDiagram
    autonumber
    actor User as User
    participant MW as MainWindow (GUI Thread)
    participant SE as ScannerEngine (Worker Thread)
    participant FS as Win32 Filesystem API
    participant DN as DiskNode Memory Tree

    User->>MW: Clicks "Scan Now" or selects drive
    MW->>SE: startScan(targetPath)
    activate SE
    SE->>SE: Initialize atomic flags (cancel=false, scanning=true)
    SE->>MW: emit scanStarted()
    
    loop Directory Traversal (Recursive DFS)
        SE->>FS: fs::directory_iterator(path, skip_permission_denied)
        FS-->>SE: directory_entry (metadata, timestamps, junction attributes)
        SE->>DN: Construct DiskNode & set attributes
        alt Throttle Interval Elapsed (> 60ms)
            SE->>MW: emit scanProgress(scannedFiles, scannedBytes, currentDir)
            MW->>MW: Update status bar metrics
        end
    end

    SE->>DN: rootNode->calculateBottomUpSizes()
    Note over DN: Post-order traversal sums sizes & rolls up newest modification timestamps
    SE->>DN: rootNode->sortChildrenBySize()
    Note over DN: Descending sort by storage consumption

    SE->>MW: emit scanFinished(rootNode, elapsedMs, cancelled)
    deactivate SE
    MW->>MW: Synchronize TreeView, Treemap, Sunburst, TopFiles, & Extension stats
```

---

## 3. Core Class & Domain Model

```mermaid
classDiagram
    class DiskNode {
        -QString m_name
        -QString m_path
        -int64_t m_size
        -int64_t m_lastModifiedTime
        -int64_t m_fileCount
        -int64_t m_dirCount
        -bool m_isDir
        -DiskNode* m_parent
        -vector~unique_ptr~DiskNode~~ m_children
        +calculateBottomUpSizes() void
        +sortChildrenBySize() void
        +getColorForExtension(QString ext)$ QColor
        +getColorForAge(int64_t lastModifiedSec)$ QColor
        +formatAge(int64_t lastModifiedSec)$ QString
        +formatSize(int64_t bytes)$ QString
    }

    class ScannerEngine {
        -QAtomicInt m_isScanning
        -QAtomicInt m_cancelRequested
        -QString m_targetPath
        +startScan(QString path) void
        +cancelScan() void
        -scanDirectory(QString path, int depth) unique_ptr~DiskNode~
    }

    class TreemapWidget {
        -DiskNode* m_currentRoot
        -ColorMode m_colorMode
        -QVariantAnimation* m_zoomAnim
        -QPixmap m_prevPixmap
        -QPixmap m_nextPixmap
        +setColorMode(ColorMode mode) void
        +zoomIn(DiskNode* node) void
        +zoomOut() void
        -renderTreemap(QPainter* painter, QRectF bounds) void
    }

    class SunburstWidget {
        -DiskNode* m_currentRoot
        -ColorMode m_colorMode
        -vector~SunburstSlice~ m_slices
        +setColorMode(ColorMode mode) void
        +zoomIn(DiskNode* node) void
        +zoomOut() void
        -computeLayout() void
    }

    class SnapshotEngine {
        +saveSnapshot(DiskNode* root, QString path)$ bool
        +loadSnapshot(QString path)$ unique_ptr~DiskNode~
        +compareTrees(DiskNode* oldRoot, DiskNode* newRoot, DiffSummary& summary)$ unique_ptr~DiskNode~
    }

    class ReportExporter {
        +exportHtml(DiskNode* root, QString filePath)$ bool
        +exportCsv(DiskNode* root, QString filePath)$ bool
        +exportJson(DiskNode* root, QString filePath)$ bool
    }

    DiskNode "1" *-- "*" DiskNode : contains children
    ScannerEngine ..> DiskNode : constructs
    TreemapWidget --> DiskNode : visualizes
    SunburstWidget --> DiskNode : visualizes
    SnapshotEngine ..> DiskNode : serializes / diffs
    ReportExporter ..> DiskNode : traverses
```
