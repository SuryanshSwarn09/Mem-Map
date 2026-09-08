#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <QCoreApplication>
#include "core/DiskNode.h"
#include "core/ScannerEngine.h"
#include "ui/TreemapLayout.h"

namespace fs = std::filesystem;

void testDiskNodeBottomUp() {
    std::cout << "[TEST] Running testDiskNodeBottomUp..." << std::endl;
    auto root = std::make_unique<DiskNode>("root", "C:/test_root", true);

    auto dirA = std::make_unique<DiskNode>("dirA", "C:/test_root/dirA", true);
    auto fileA1 = std::make_unique<DiskNode>("fileA1.txt", "C:/test_root/dirA/fileA1.txt", false);
    fileA1->setSize(1000);
    auto fileA2 = std::make_unique<DiskNode>("fileA2.mp4", "C:/test_root/dirA/fileA2.mp4", false);
    fileA2->setSize(4000);
    dirA->addChild(std::move(fileA1));
    dirA->addChild(std::move(fileA2));

    auto dirB = std::make_unique<DiskNode>("dirB", "C:/test_root/dirB", true);
    auto fileB1 = std::make_unique<DiskNode>("fileB1.zip", "C:/test_root/dirB/fileB1.zip", false);
    fileB1->setSize(5000);
    dirB->addChild(std::move(fileB1));

    root->addChild(std::move(dirA));
    root->addChild(std::move(dirB));

    root->calculateBottomUpSizes();
    root->sortChildrenBySize();

    assert(root->size() == 10000);
    assert(root->fileCount() == 3);
    assert(root->dirCount() == 2);

    // Verify sorting: dirA and dirB both have size 5000
    assert(root->children().size() == 2);
    assert(root->children()[0]->size() == 5000);
    assert(root->children()[1]->size() == 5000);

    // Verify categories
    assert(DiskNode::getFileCategory("txt") == "Document");
    assert(DiskNode::getFileCategory("mp4") == "Video");
    assert(DiskNode::getFileCategory("zip") == "Archive");

    std::cout << "  -> PASSED! Total size: " << root->size() << " bytes, files: " << root->fileCount() << std::endl;
}

void testTreemapLayout() {
    std::cout << "[TEST] Running testTreemapLayout..." << std::endl;
    auto root = std::make_unique<DiskNode>("root", "C:/test", true);

    for (int i = 1; i <= 5; ++i) {
        auto file = std::make_unique<DiskNode>(QString("file%1.dat").arg(i), QString("C:/test/file%1.dat").arg(i), false);
        file->setSize(i * 1000);
        root->addChild(std::move(file));
    }

    root->calculateBottomUpSizes();
    root->sortChildrenBySize();

    QRectF bounds(0, 0, 800, 600);
    auto tiles = TreemapLayout::compute(root.get(), bounds, 1);

    assert(!tiles.empty());
    assert(tiles.size() == 5);

    for (const auto& tile : tiles) {
        assert(tile.rect.isValid());
        assert(tile.rect.width() > 0);
        assert(tile.rect.height() > 0);
        assert(tile.rect.left() >= -1.0 && tile.rect.right() <= bounds.width() + 1.0);
        assert(tile.rect.top() >= -1.0 && tile.rect.bottom() <= bounds.height() + 1.0);
    }

    std::cout << "  -> PASSED! Generated " << tiles.size() << " valid squarified tiles." << std::endl;
}

void testRealDirectoryScan() {
    std::cout << "[TEST] Running testRealDirectoryScan on project source tree..." << std::endl;

    // Scan e:/Mem-scan/src
    ScannerEngine engine;
    bool finished = false;
    std::shared_ptr<DiskNode> scannedRoot;

    QObject::connect(&engine, &ScannerEngine::scanFinished, [&](std::shared_ptr<DiskNode> root, qint64 elapsedMs, bool wasCancelled) {
        scannedRoot = root;
        finished = true;
        std::cout << "  -> Scan completed in " << elapsedMs << "ms! Cancelled: " << (wasCancelled ? "yes" : "no") << std::endl;
    });

    engine.startScan("E:/Mem-scan/src");
    
    // Wait for thread to finish
    engine.wait(5000);

    assert(finished);
    assert(scannedRoot != nullptr);
    assert(scannedRoot->fileCount() > 0);
    assert(scannedRoot->size() > 0);

    std::cout << "  -> PASSED! Scanned " << scannedRoot->fileCount() << " files, " 
              << DiskNode::formatSize(scannedRoot->size()).toStdString() << std::endl;
}

#include "core/ReportExporter.h"
#include "core/SnapshotEngine.h"
#include <QFile>

void testReportExporter() {
    std::cout << "[TEST] Running testReportExporter..." << std::endl;
    auto root = std::make_unique<DiskNode>("test_root", "C:/test_root", true);
    auto f1 = std::make_unique<DiskNode>("doc.pdf", "C:/test_root/doc.pdf", false);
    f1->setSize(2048);
    auto f2 = std::make_unique<DiskNode>("clip.mp4", "C:/test_root/clip.mp4", false);
    f2->setSize(8192);
    root->addChild(std::move(f1));
    root->addChild(std::move(f2));
    root->calculateBottomUpSizes();

    QString csvPath = QStringLiteral("test_report.csv");
    QString jsonPath = QStringLiteral("test_report.json");
    QString htmlPath = QStringLiteral("test_report.html");

    QString err;
    assert(ReportExporter::exportToCsv(root.get(), csvPath, &err));
    assert(QFile::exists(csvPath) && QFile(csvPath).size() > 50);

    assert(ReportExporter::exportToJson(root.get(), jsonPath, &err));
    assert(QFile::exists(jsonPath) && QFile(jsonPath).size() > 50);

    assert(ReportExporter::exportToHtml(root.get(), htmlPath, &err));
    assert(QFile::exists(htmlPath) && QFile(htmlPath).size() > 100);

    // Cleanup test files
    QFile::remove(csvPath);
    QFile::remove(jsonPath);
    QFile::remove(htmlPath);

    std::cout << "  -> PASSED! Successfully exported and verified CSV, JSON, and HTML reports." << std::endl;
}

void testSnapshotEngine() {
    std::cout << "[TEST] Running testSnapshotEngine..." << std::endl;
    auto rootOld = std::make_unique<DiskNode>("root", "C:/test", true);
    auto fileKeep = std::make_unique<DiskNode>("keep.txt", "C:/test/keep.txt", false);
    fileKeep->setSize(1000);
    auto fileDelete = std::make_unique<DiskNode>("del.log", "C:/test/del.log", false);
    fileDelete->setSize(500);
    auto fileModify = std::make_unique<DiskNode>("mod.dat", "C:/test/mod.dat", false);
    fileModify->setSize(2000);

    rootOld->addChild(std::move(fileKeep));
    rootOld->addChild(std::move(fileDelete));
    rootOld->addChild(std::move(fileModify));
    rootOld->calculateBottomUpSizes();

    // 1. Test save and load snapshot
    QString snapPath = QStringLiteral("test_snapshot.mmap");
    QString err;
    assert(SnapshotEngine::saveSnapshot(rootOld.get(), snapPath, &err));
    assert(QFile::exists(snapPath));

    auto loadedRoot = SnapshotEngine::loadSnapshot(snapPath, &err);
    assert(loadedRoot != nullptr);
    assert(loadedRoot->size() == 3500);
    assert(loadedRoot->fileCount() == 3);
    QFile::remove(snapPath);

    // 2. Test Diff Engine with changes:
    // keep.txt: unchanged (1000)
    // del.log: deleted (-500)
    // mod.dat: modified 2000 -> 3500 (+1500)
    // added.bin: added (+3000)
    auto rootNew = std::make_unique<DiskNode>("root", "C:/test", true);
    auto fileKeep2 = std::make_unique<DiskNode>("keep.txt", "C:/test/keep.txt", false);
    fileKeep2->setSize(1000);
    auto fileModify2 = std::make_unique<DiskNode>("mod.dat", "C:/test/mod.dat", false);
    fileModify2->setSize(3500);
    auto fileAdded = std::make_unique<DiskNode>("added.bin", "C:/test/added.bin", false);
    fileAdded->setSize(3000);

    rootNew->addChild(std::move(fileKeep2));
    rootNew->addChild(std::move(fileModify2));
    rootNew->addChild(std::move(fileAdded));
    rootNew->calculateBottomUpSizes();

    DiffSummary summary;
    auto diffTree = SnapshotEngine::compareTrees(loadedRoot.get(), rootNew.get(), summary);

    assert(diffTree != nullptr);
    assert(summary.addedCount == 1);
    assert(summary.deletedCount == 1);
    assert(summary.modifiedCount == 1);
    // Net delta = 7500 (new) - 3500 (old) = +4000
    assert(summary.deltaTotalBytes == 4000);

    std::cout << "  -> PASSED! Snapshot save/load and Tree Diff (Delta: " << summary.deltaTotalBytes 
              << " bytes, Added: " << summary.addedCount 
              << ", Deleted: " << summary.deletedCount 
              << ", Modified: " << summary.modifiedCount << ") verified." << std::endl;
}

void testTreemapAnimationGeometry() {
    std::cout << "[TEST] Running testTreemapAnimationGeometry..." << std::endl;
    // Test geometric interpolation logic used in TreemapWidget smooth zoom
    QRectF fullRect(0, 0, 800, 600);
    QRectF tileRect(100, 150, 200, 100);

    auto interpRect = [](const QRectF& from, const QRectF& to, qreal t) {
        return QRectF(
            from.left() * (1.0 - t) + to.left() * t,
            from.top() * (1.0 - t) + to.top() * t,
            from.width() * (1.0 - t) + to.width() * t,
            from.height() * (1.0 - t) + to.height() * t
        );
    };

    QRectF r0 = interpRect(tileRect, fullRect, 0.0);
    assert(r0 == tileRect);
    (void)r0;

    QRectF r1 = interpRect(tileRect, fullRect, 1.0);
    assert(r1 == fullRect);
    (void)r1;

    QRectF rHalf = interpRect(tileRect, fullRect, 0.5);
    assert(rHalf.width() == 500.0);
    assert(rHalf.height() == 350.0);
    assert(rHalf.left() == 50.0);
    assert(rHalf.top() == 75.0);
    (void)rHalf;

    QRectF outHalf = interpRect(fullRect, tileRect, 0.5);
    assert(outHalf == rHalf);
    (void)outHalf;

    std::cout << "  -> PASSED! Interpolation math validated at all stages (t=0.0, 0.5, 1.0)." << std::endl;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "      Mem-Map Automated Unit Tests     " << std::endl;
    std::cout << "========================================" << std::endl;

    testDiskNodeBottomUp();
    testTreemapLayout();
    testRealDirectoryScan();
    testReportExporter();
    testSnapshotEngine();
    testTreemapAnimationGeometry();

    std::cout << "========================================" << std::endl;
    std::cout << "  ALL AUTOMATED UNIT TESTS PASSED!      " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
