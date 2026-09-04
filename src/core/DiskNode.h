#pragma once

#include <QString>
#include <QColor>
#include <vector>
#include <memory>
#include <cstdint>

class DiskNode {
public:
    DiskNode(const QString& name, const QString& fullPath, bool isDir, DiskNode* parent = nullptr);
    ~DiskNode() = default;

    // Non-copyable, movable
    DiskNode(const DiskNode&) = delete;
    DiskNode& operator=(const DiskNode&) = delete;
    DiskNode(DiskNode&&) = default;
    DiskNode& operator=(DiskNode&&) = default;

    const QString& name() const { return m_name; }
    const QString& fullPath() const { return m_fullPath; }
    bool isDirectory() const { return m_isDir; }
    uint64_t size() const { return m_size; }
    uint64_t fileCount() const { return m_fileCount; }
    uint64_t dirCount() const { return m_dirCount; }
    const QString& extension() const { return m_extension; }
    DiskNode* parent() const { return m_parent; }
    const std::vector<std::unique_ptr<DiskNode>>& children() const { return m_children; }
    std::vector<std::unique_ptr<DiskNode>>& mutableChildren() { return m_children; }

    void setSize(uint64_t bytes) { m_size = bytes; }
    void setFileCount(uint64_t count) { m_fileCount = count; }
    void setDirCount(uint64_t count) { m_dirCount = count; }
    void setParent(DiskNode* parent) { m_parent = parent; }

    DiskNode* addChild(std::unique_ptr<DiskNode> child);
    void calculateBottomUpSizes();
    void sortChildrenBySize();

    int row() const;
    DiskNode* findNodeByPath(const QString& path);

    // Static Helpers
    static QString formatSize(uint64_t bytes);
    static QString getFileCategory(const QString& ext);
    static QColor getColorForCategory(const QString& category);
    static QColor getColorForExtension(const QString& ext);

private:
    QString m_name;
    QString m_fullPath;
    bool m_isDir;
    uint64_t m_size{0};
    uint64_t m_fileCount{0};
    uint64_t m_dirCount{0};
    QString m_extension;
    DiskNode* m_parent{nullptr};
    std::vector<std::unique_ptr<DiskNode>> m_children;
};
