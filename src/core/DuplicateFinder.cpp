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

std::vector<std::vector<DiskNode*>> filterCandidateSizeGroups(
    const std::unordered_map<int64_t, std::vector<DiskNode*>>& sizeMap) 
{
    std::vector<std::vector<DiskNode*>> candidateGroups;
    for (const auto& [size, files] : sizeMap) {
        if (files.size() >= 2) {
            candidateGroups.push_back(files);
        }
    }
    std::sort(candidateGroups.begin(), candidateGroups.end(),
              [](const std::vector<DiskNode*>& a, const std::vector<DiskNode*>& b) {
                  return (!a.empty() && !b.empty()) ? a.front()->size() > b.front()->size() : false;
              });
    return candidateGroups;
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
