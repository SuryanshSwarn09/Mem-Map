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

QByteArray computePartialHeaderHash(const QString& filePath, qint64 maxBytes = 4096) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray header = file.read(maxBytes);
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(header);
    return hash.result();
}

QString computeFullFileHash(const QString& filePath, 
                            QCryptographicHash::Algorithm algo,
                            const std::atomic<bool>* cancelFlag) 
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(algo);
    constexpr qint64 bufferSize = 65536; // 64 KB streaming buffer
    QByteArray buffer;
    buffer.resize(bufferSize);

    while (!file.atEnd()) {
        if (cancelFlag && cancelFlag->load()) {
            return QString();
        }
        qint64 bytesRead = file.read(buffer.data(), bufferSize);
        if (bytesRead > 0) {
            hash.addData(buffer.constData(), static_cast<int>(bytesRead));
        } else if (bytesRead < 0) {
            return QString();
        }
    }

    return QString::fromLatin1(hash.result().toHex());
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

DuplicateScanResult DuplicateFinder::scanDuplicates(DiskNode* rootNode,
                                                    HashAlgorithm algo,
                                                    std::atomic<bool>* cancelFlag)
{
    DuplicateScanResult result;
    if (!rootNode) return result;

    QElapsedTimer timer;
    timer.start();

    // Pass 1: Collect all regular files into byte-size buckets
    std::unordered_map<int64_t, std::vector<DiskNode*>> sizeMap;
    collectFilesBySize(rootNode, sizeMap);

    if (cancelFlag && cancelFlag->load()) return result;

    // Filter candidate groups with >= 2 files
    auto candidateGroups = filterCandidateSizeGroups(sizeMap);
    
    int64_t totalCandidateFiles = 0;
    for (const auto& group : candidateGroups) {
        totalCandidateFiles += group.size();
    }
    result.scannedCandidates = totalCandidateFiles;

    if (totalCandidateFiles == 0) {
        result.elapsedMs = timer.elapsed();
        return result;
    }

    QCryptographicHash::Algorithm hashAlgo = (algo == HashAlgorithm::Sha256) 
        ? QCryptographicHash::Sha256 
        : QCryptographicHash::Md5;

    // Pass 2 & 3: For each size bucket, group by 4KB header, then full hash
    for (const auto& sizeGroup : candidateGroups) {
        if (cancelFlag && cancelFlag->load()) return result;

        // Group by 4KB header hash first
        std::unordered_map<std::string, std::vector<DiskNode*>> headerMap;
        for (DiskNode* node : sizeGroup) {
            if (cancelFlag && cancelFlag->load()) return result;
            QByteArray h = computePartialHeaderHash(node->fullPath());
            if (!h.isEmpty()) {
                headerMap[h.toStdString()].push_back(node);
            }
        }

        // For each matching header bucket with >= 2 files, compute full hash
        for (const auto& [headerKey, headerFiles] : headerMap) {
            if (headerFiles.size() < 2) continue;
            if (cancelFlag && cancelFlag->load()) return result;

            std::unordered_map<QString, std::vector<DiskNode*>> fullHashMap;
            for (DiskNode* node : headerFiles) {
                if (cancelFlag && cancelFlag->load()) return result;
                QString fullHash = computeFullFileHash(node->fullPath(), hashAlgo, cancelFlag);
                if (!fullHash.isEmpty()) {
                    fullHashMap[fullHash].push_back(node);
                }
            }

            // Record confirmed duplicate groups
            for (const auto& [hashStr, dupNodes] : fullHashMap) {
                if (dupNodes.size() >= 2) {
                    DuplicateGroup group;
                    group.hash = hashStr;
                    group.fileSize = dupNodes.front()->size();
                    for (DiskNode* dn : dupNodes) {
                        DuplicateFile df;
                        df.path = dn->fullPath();
                        df.size = dn->size();
                        df.lastModified = dn->lastModifiedTime();
                        df.isSelectedForDeletion = false;
                        group.files.push_back(df);
                    }
                    std::sort(group.files.begin(), group.files.end(),
                              [](const DuplicateFile& a, const DuplicateFile& b) {
                                  return a.path < b.path;
                              });

                    result.totalWastedBytes += group.wastedBytes();
                    result.totalDuplicateFiles += group.files.size();
                    result.groups.push_back(std::move(group));
                }
            }
        }
    }

    // Sort duplicate groups by wasted space descending
    std::sort(result.groups.begin(), result.groups.end(),
              [](const DuplicateGroup& a, const DuplicateGroup& b) {
                  return a.wastedBytes() > b.wastedBytes();
              });
    result.totalGroups = static_cast<int64_t>(result.groups.size());
    result.elapsedMs = timer.elapsed();

    return result;
}

void DuplicateFinder::run() {
    emit scanStarted();

    DuplicateScanResult result = scanDuplicates(m_rootNode, m_algorithm, &m_cancelRequested);

    m_isScanning.store(false);

    if (m_cancelRequested.load()) {
        emit scanCancelled();
    } else {
        emit scanFinished(result);
    }
}
