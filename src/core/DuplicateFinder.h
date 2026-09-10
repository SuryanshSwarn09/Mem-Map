#pragma once

#include <QString>
#include <cstdint>
#include <vector>
#include <atomic>
#include <memory>
#include <QThread>
#include "DiskNode.h"

struct DuplicateFile {
    QString path;
    int64_t size{0};
    int64_t lastModified{0};
    bool isSelectedForDeletion{false};
};

struct DuplicateGroup {
    QString hash;
    int64_t fileSize{0};
    std::vector<DuplicateFile> files;

    int64_t wastedBytes() const {
        return files.size() > 1 ? (static_cast<int64_t>(files.size()) - 1) * fileSize : 0;
    }
};

enum class HashAlgorithm {
    Md5,
    Sha256
};

struct DuplicateScanResult {
    std::vector<DuplicateGroup> groups;
    int64_t totalWastedBytes{0};
    int64_t totalDuplicateFiles{0};
    int64_t totalGroups{0};
    int64_t scannedCandidates{0};
    int64_t elapsedMs{0};
};

class DuplicateFinder : public QThread {
    Q_OBJECT

public:
    explicit DuplicateFinder(QObject* parent = nullptr);
    ~DuplicateFinder() override;

    void startDuplicateScan(DiskNode* rootNode, HashAlgorithm algo = HashAlgorithm::Md5);
    void cancelScan();
    bool isScanning() const { return m_isScanning.load(); }

    static DuplicateScanResult scanDuplicates(DiskNode* rootNode,
                                              HashAlgorithm algo = HashAlgorithm::Md5,
                                              std::atomic<bool>* cancelFlag = nullptr);

signals:
    void scanStarted();
    void scanProgress(int current, int total, const QString& currentFile);
    void scanFinished(const DuplicateScanResult& result);
    void scanCancelled();

protected:
    void run() override;

private:
    DiskNode* m_rootNode{nullptr};
    HashAlgorithm m_algorithm{HashAlgorithm::Md5};
    std::atomic<bool> m_isScanning{false};
    std::atomic<bool> m_cancelRequested{false};
};
