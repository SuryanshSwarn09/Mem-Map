#include "SnapshotEngine.h"
#include <QFile>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <map>
#include <cmath>
#include <algorithm>

DiffNode* DiffNode::addChild(std::unique_ptr<DiffNode> child) {
    if (!child) return nullptr;
    child->parent = this;
    DiffNode* raw = child.get();
    children.push_back(std::move(child));
    return raw;
}

void DiffNode::sortChildrenByAbsoluteDelta() {
    std::sort(children.begin(), children.end(), [](const std::unique_ptr<DiffNode>& a, const std::unique_ptr<DiffNode>& b) {
        return std::abs(a->deltaBytes) > std::abs(b->deltaBytes);
    });

    for (const auto& child : children) {
        if (child->isDir) {
            child->sortChildrenByAbsoluteDelta();
        }
    }
}

QString DiffNode::statusToString(DiffStatus st) {
    switch (st) {
        case DiffStatus::Added:    return QStringLiteral("Added");
        case DiffStatus::Deleted:  return QStringLiteral("Deleted");
        case DiffStatus::Modified: return QStringLiteral("Modified");
        case DiffStatus::Unchanged:return QStringLiteral("Unchanged");
    }
    return QStringLiteral("Unknown");
}

static QJsonObject nodeToJson(const DiskNode* node) {
    QJsonObject obj;
    if (!node) return obj;

    obj[QStringLiteral("name")] = node->name();
    obj[QStringLiteral("path")] = node->fullPath();
    obj[QStringLiteral("isDir")] = node->isDirectory();
    obj[QStringLiteral("size")] = static_cast<qint64>(node->size());
    obj[QStringLiteral("files")] = static_cast<qint64>(node->fileCount());
    obj[QStringLiteral("folders")] = static_cast<qint64>(node->dirCount());

    if (!node->isDirectory()) {
        obj[QStringLiteral("ext")] = node->extension();
    } else {
        QJsonArray arr;
        for (const auto& c : node->children()) {
            arr.append(nodeToJson(c.get()));
        }
        obj[QStringLiteral("children")] = arr;
    }

    return obj;
}

bool SnapshotEngine::saveSnapshot(const DiskNode* root, const QString& filePath, QString* errorMsg) {
    if (!root) {
        if (errorMsg) *errorMsg = QStringLiteral("Root node is null.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMsg) *errorMsg = file.errorString();
        return false;
    }

    QJsonObject meta;
    meta[QStringLiteral("app")] = QStringLiteral("Mem-Map");
    meta[QStringLiteral("format")] = QStringLiteral("mmap_snapshot_v1");
    meta[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    meta[QStringLiteral("rootPath")] = root->fullPath();
    meta[QStringLiteral("totalSize")] = static_cast<qint64>(root->size());
    meta[QStringLiteral("fileCount")] = static_cast<qint64>(root->fileCount());
    meta[QStringLiteral("dirCount")] = static_cast<qint64>(root->dirCount());
    meta[QStringLiteral("root")] = nodeToJson(root);

    QJsonDocument doc(meta);
    file.write(doc.toJson(QJsonDocument::Compact));
    return true;
}

std::unique_ptr<DiskNode> SnapshotEngine::jsonObjectToNode(const QJsonObject& obj, DiskNode* parent) {
    QString name = obj[QStringLiteral("name")].toString();
    QString path = obj[QStringLiteral("path")].toString();
    bool isDir = obj[QStringLiteral("isDir")].toBool();

    auto node = std::make_unique<DiskNode>(name, path, isDir, parent);
    node->setSize(static_cast<uint64_t>(obj[QStringLiteral("size")].toVariant().toULongLong()));
    node->setFileCount(static_cast<uint64_t>(obj[QStringLiteral("files")].toVariant().toULongLong()));
    node->setDirCount(static_cast<uint64_t>(obj[QStringLiteral("folders")].toVariant().toULongLong()));

    if (isDir && obj.contains(QStringLiteral("children"))) {
        QJsonArray arr = obj[QStringLiteral("children")].toArray();
        for (const auto& val : arr) {
            auto childNode = jsonObjectToNode(val.toObject(), node.get());
            if (childNode) {
                node->addChild(std::move(childNode));
            }
        }
    }

    return node;
}

std::unique_ptr<DiskNode> SnapshotEngine::loadSnapshot(const QString& filePath, QString* errorMsg) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg) *errorMsg = file.errorString();
        return nullptr;
    }

    QByteArray data = file.readAll();
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMsg) *errorMsg = QStringLiteral("Invalid snapshot JSON format: ") + parseErr.errorString();
        return nullptr;
    }

    QJsonObject rootObj = doc.object();
    QJsonObject treeObj = rootObj[QStringLiteral("root")].toObject();
    if (treeObj.isEmpty()) {
        if (errorMsg) *errorMsg = QStringLiteral("Snapshot contains no root tree.");
        return nullptr;
    }

    auto rootNode = jsonObjectToNode(treeObj, nullptr);
    if (rootNode) {
        rootNode->calculateBottomUpSizes();
        rootNode->sortChildrenBySize();
    }
    return rootNode;
}

std::unique_ptr<DiffNode> SnapshotEngine::createWholeSubtreeDiff(const DiskNode* node, DiffStatus status, DiffNode* parentDiff, DiffSummary& summary) {
    if (!node) return nullptr;

    auto diff = std::make_unique<DiffNode>();
    diff->name = node->name();
    diff->fullPath = node->fullPath();
    diff->isDir = node->isDirectory();
    diff->parent = parentDiff;
    diff->status = status;

    if (status == DiffStatus::Added) {
        diff->oldSize = 0;
        diff->newSize = node->size();
        diff->deltaBytes = static_cast<int64_t>(node->size());
        if (!node->isDirectory()) summary.addedCount++;
    } else { // Deleted
        diff->oldSize = node->size();
        diff->newSize = 0;
        diff->deltaBytes = -static_cast<int64_t>(node->size());
        if (!node->isDirectory()) summary.deletedCount++;
    }

    if (node->isDirectory()) {
        for (const auto& child : node->children()) {
            auto childDiff = createWholeSubtreeDiff(child.get(), status, diff.get(), summary);
            if (childDiff) {
                diff->addChild(std::move(childDiff));
            }
        }
    }

    return diff;
}

std::unique_ptr<DiffNode> SnapshotEngine::compareNodes(const DiskNode* oldNode, const DiskNode* newNode, DiffNode* parentDiff, DiffSummary& summary) {
    if (!oldNode && !newNode) return nullptr;

    if (!oldNode) {
        return createWholeSubtreeDiff(newNode, DiffStatus::Added, parentDiff, summary);
    }
    if (!newNode) {
        return createWholeSubtreeDiff(oldNode, DiffStatus::Deleted, parentDiff, summary);
    }

    auto diff = std::make_unique<DiffNode>();
    diff->name = newNode->name();
    diff->fullPath = newNode->fullPath();
    diff->isDir = newNode->isDirectory();
    diff->parent = parentDiff;
    diff->oldSize = oldNode->size();
    diff->newSize = newNode->size();
    diff->deltaBytes = static_cast<int64_t>(newNode->size()) - static_cast<int64_t>(oldNode->size());

    if (!diff->isDir) {
        if (diff->deltaBytes != 0) {
            diff->status = DiffStatus::Modified;
            summary.modifiedCount++;
        } else {
            diff->status = DiffStatus::Unchanged;
        }
        return diff;
    }

    // Both are directories: compare children by name
    std::map<QString, const DiskNode*> oldChildren;
    for (const auto& c : oldNode->children()) {
        oldChildren[c->name()] = c.get();
    }

    std::map<QString, const DiskNode*> newChildren;
    for (const auto& c : newNode->children()) {
        newChildren[c->name()] = c.get();
    }

    // Check all children in newNode
    for (const auto& [name, nChild] : newChildren) {
        auto it = oldChildren.find(name);
        if (it != oldChildren.end()) {
            // Child exists in both
            auto childDiff = compareNodes(it->second, nChild, diff.get(), summary);
            if (childDiff && childDiff->status != DiffStatus::Unchanged) {
                diff->addChild(std::move(childDiff));
            }
        } else {
            // Child was Added
            auto childDiff = createWholeSubtreeDiff(nChild, DiffStatus::Added, diff.get(), summary);
            if (childDiff) {
                diff->addChild(std::move(childDiff));
            }
        }
    }

    // Check children in oldNode that no longer exist in newNode (Deleted)
    for (const auto& [name, oChild] : oldChildren) {
        if (newChildren.find(name) == newChildren.end()) {
            auto childDiff = createWholeSubtreeDiff(oChild, DiffStatus::Deleted, diff.get(), summary);
            if (childDiff) {
                diff->addChild(std::move(childDiff));
            }
        }
    }

    if (diff->deltaBytes != 0 || !diff->children.empty()) {
        diff->status = DiffStatus::Modified;
    } else {
        diff->status = DiffStatus::Unchanged;
    }

    return diff;
}

std::unique_ptr<DiffNode> SnapshotEngine::compareTrees(const DiskNode* oldRoot, const DiskNode* newRoot, DiffSummary& outSummary) {
    outSummary = DiffSummary{};

    if (!oldRoot && !newRoot) return nullptr;

    if (oldRoot) {
        outSummary.oldPath = oldRoot->fullPath();
        outSummary.oldTotalBytes = oldRoot->size();
    }
    if (newRoot) {
        outSummary.newPath = newRoot->fullPath();
        outSummary.newTotalBytes = newRoot->size();
    }

    outSummary.deltaTotalBytes = static_cast<int64_t>(outSummary.newTotalBytes) - static_cast<int64_t>(outSummary.oldTotalBytes);

    auto rootDiff = compareNodes(oldRoot, newRoot, nullptr, outSummary);
    if (rootDiff) {
        rootDiff->sortChildrenByAbsoluteDelta();
    }

    return rootDiff;
}
