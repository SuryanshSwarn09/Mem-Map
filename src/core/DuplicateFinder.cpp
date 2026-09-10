#include "DuplicateFinder.h"
#include <unordered_map>
#include <QElapsedTimer>
#include <QFile>
#include <QCryptographicHash>
#include <algorithm>

namespace {
void collectFilesBySize(DiskNode* node, std::unordered_map<int64_t, std::vector<DiskNode*>>& sizeMap) {
    if (!node) return;

    if (!node->isDirectory()) {
        if (node->size() > 0) {
            sizeMap[node->size()].push_back(node);
        }
    } else {
        for (size_t i = 0; i < node->childCount(); ++i) {
            collectFilesBySize(node->child(i), sizeMap);
        }
    }
}
} // anonymous namespace

DuplicateFinder::DuplicateFinder(QObject* parent)
    : QThread(parent)
{
}

DuplicateFinder::~DuplicateFinder() {
    cancelScan();
    wait();
}

void DuplicateFinder::startDuplicateScan(DiskNode* rootNode, HashAlgorithm algo) {
    if (m_isScanning.load()) {
        cancelScan();
        wait();
    }

    m_rootNode = rootNode;
    m_algorithm = algo;
    m_cancelRequested.store(false);
    m_isScanning.store(true);
    start();
}

void DuplicateFinder::cancelScan() {
    m_cancelRequested.store(true);
}
