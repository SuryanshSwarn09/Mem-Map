#pragma once

#include <QString>
#include <QJsonObject>
#include <vector>
#include <memory>
#include <cstdint>
#include "DiskNode.h"

enum class DiffStatus {
    Added,
    Deleted,
    Modified,
    Unchanged
};

struct DiffNode {
    QString name;
    QString fullPath;
    bool isDir{false};
    uint64_t oldSize{0};
    uint64_t newSize{0};
    int64_t deltaBytes{0};
    DiffStatus status{DiffStatus::Unchanged};

    DiffNode* parent{nullptr};
    std::vector<std::unique_ptr<DiffNode>> children;

    DiffNode* addChild(std::unique_ptr<DiffNode> child);
    void sortChildrenByAbsoluteDelta();
    static QString statusToString(DiffStatus st);
};

struct DiffSummary {
    QString oldPath;
    QString newPath;
    QString oldTimestamp;
    QString newTimestamp;
    uint64_t oldTotalBytes{0};
    uint64_t newTotalBytes{0};
    int64_t deltaTotalBytes{0};
    uint64_t addedCount{0};
    uint64_t deletedCount{0};
    uint64_t modifiedCount{0};
};

class SnapshotEngine {
public:
    static bool saveSnapshot(const DiskNode* root, const QString& filePath, QString* errorMsg = nullptr);
    static std::unique_ptr<DiskNode> loadSnapshot(const QString& filePath, QString* errorMsg = nullptr);

    static std::unique_ptr<DiffNode> compareTrees(const DiskNode* oldRoot, const DiskNode* newRoot, DiffSummary& outSummary);

private:
    static std::unique_ptr<DiskNode> jsonObjectToNode(const QJsonObject& obj, DiskNode* parent);
    static std::unique_ptr<DiffNode> compareNodes(const DiskNode* oldNode, const DiskNode* newNode, DiffNode* parentDiff, DiffSummary& summary);
    static std::unique_ptr<DiffNode> createWholeSubtreeDiff(const DiskNode* node, DiffStatus status, DiffNode* parentDiff, DiffSummary& summary);
};
