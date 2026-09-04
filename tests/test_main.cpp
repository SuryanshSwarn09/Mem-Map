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

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "      Mem-Map Automated Unit Tests     " << std::endl;
    std::cout << "========================================" << std::endl;

    testDiskNodeBottomUp();
    testTreemapLayout();
    testRealDirectoryScan();

    std::cout << "========================================" << std::endl;
    std::cout << "  ALL AUTOMATED UNIT TESTS PASSED!      " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
