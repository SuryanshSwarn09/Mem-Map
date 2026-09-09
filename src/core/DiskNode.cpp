#include "DiskNode.h"
#include <QFileInfo>
#include <algorithm>

DiskNode::DiskNode(const QString& name, const QString& fullPath, bool isDir, DiskNode* parent)
    : m_name(name)
    , m_fullPath(fullPath)
    , m_isDir(isDir)
    , m_parent(parent)
{
    if (!m_isDir) {
        int dotIdx = m_name.lastIndexOf(QLatin1Char('.'));
        if (dotIdx >= 0 && dotIdx < m_name.length() - 1) {
            m_extension = m_name.mid(dotIdx + 1).toLower();
        } else {
            m_extension = QStringLiteral("(none)");
        }
    }
}

DiskNode* DiskNode::addChild(std::unique_ptr<DiskNode> child) {
    if (!child) return nullptr;
    child->setParent(this);
    DiskNode* raw = child.get();
    m_children.push_back(std::move(child));
    return raw;
}

void DiskNode::calculateBottomUpSizes() {
    if (!m_isDir) {
        m_fileCount = 1;
        m_dirCount = 0;
        return;
    }

    m_size = 0;
    m_fileCount = 0;
    m_dirCount = 0;

    for (const auto& child : m_children) {
        child->calculateBottomUpSizes();
        m_size += child->size();
        if (child->isDirectory()) {
            m_dirCount += 1 + child->dirCount();
            m_fileCount += child->fileCount();
        } else {
            m_fileCount += 1;
        }
        if (child->lastModifiedTime() > m_lastModifiedTime) {
            m_lastModifiedTime = child->lastModifiedTime();
        }
    }
}

void DiskNode::sortChildrenBySize() {
    std::sort(m_children.begin(), m_children.end(),
              [](const std::unique_ptr<DiskNode>& a, const std::unique_ptr<DiskNode>& b) {
                  return a->size() > b->size();
              });

    for (const auto& child : m_children) {
        if (child->isDirectory()) {
            child->sortChildrenBySize();
        }
    }
}

int DiskNode::row() const {
    if (!m_parent) return 0;
    const auto& siblings = m_parent->m_children;
    for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

DiskNode* DiskNode::findNodeByPath(const QString& path) {
    if (m_fullPath == path) {
        return this;
    }

    for (const auto& child : m_children) {
        if (path == child->fullPath()) {
            return child.get();
        }
        if (child->isDirectory() && path.startsWith(child->fullPath())) {
            DiskNode* match = child->findNodeByPath(path);
            if (match) return match;
        }
    }
    return nullptr;
}

QString DiskNode::formatSize(uint64_t bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    if (bytes >= TB) {
        return QString::asprintf("%.2f TB", bytes / TB);
    } else if (bytes >= GB) {
        return QString::asprintf("%.2f GB", bytes / GB);
    } else if (bytes >= MB) {
        return QString::asprintf("%.1f MB", bytes / MB);
    } else if (bytes >= KB) {
        return QString::asprintf("%.1f KB", bytes / KB);
    } else {
        return QString::asprintf("%llu B", static_cast<unsigned long long>(bytes));
    }
}

QString DiskNode::getFileCategory(const QString& ext) {
    const QString e = ext.toLower();

    // Video
    if (e == "mp4" || e == "mkv" || e == "avi" || e == "mov" || e == "wmv" || e == "flv" || e == "webm" || e == "m4v" || e == "ts")
        return QStringLiteral("Video");

    // Audio
    if (e == "mp3" || e == "wav" || e == "flac" || e == "aac" || e == "ogg" || e == "m4a" || e == "wma" || e == "opus")
        return QStringLiteral("Audio");

    // Image
    if (e == "jpg" || e == "jpeg" || e == "png" || e == "gif" || e == "bmp" || e == "webp" || e == "svg" || e == "psd" || e == "tiff" || e == "ico")
        return QStringLiteral("Image");

    // Document
    if (e == "pdf" || e == "doc" || e == "docx" || e == "xls" || e == "xlsx" || e == "ppt" || e == "pptx" || e == "txt" || e == "rtf" || e == "csv" || e == "md")
        return QStringLiteral("Document");

    // Archive
    if (e == "zip" || e == "rar" || e == "7z" || e == "tar" || e == "gz" || e == "bz2" || e == "xz" || e == "iso" || e == "dmg" || e == "cab")
        return QStringLiteral("Archive");

    // Executable / Binary
    if (e == "exe" || e == "dll" || e == "so" || e == "dylib" || e == "sys" || e == "bin" || e == "msi" || e == "app" || e == "deb" || e == "rpm")
        return QStringLiteral("Executable");

    // Code & Dev
    if (e == "cpp" || e == "c" || e == "h" || e == "hpp" || e == "py" || e == "js" || e == "ts" || e == "html" || e == "css" || e == "json" || e == "xml" || e == "java" || e == "rs" || e == "go" || e == "cs" || e == "sql" || e == "sh" || e == "bat" || e == "ps1")
        return QStringLiteral("Development");

    return QStringLiteral("Other");
}

QColor DiskNode::getColorForCategory(const QString& category) {
    if (category == "Video")       return QColor(156, 39, 176);  // Deep Purple
    if (category == "Audio")       return QColor(0, 188, 212);   // Cyan
    if (category == "Image")       return QColor(255, 152, 0);   // Amber / Orange
    if (category == "Document")    return QColor(33, 150, 243);  // Blue
    if (category == "Archive")     return QColor(255, 193, 7);   // Gold / Yellow
    if (category == "Executable")  return QColor(76, 175, 80);   // Green
    if (category == "Development") return QColor(233, 30, 99);   // Rose / Pink
    return QColor(120, 144, 156);                               // Slate Gray
}

QColor DiskNode::getColorForExtension(const QString& ext) {
    return getColorForCategory(getFileCategory(ext));
}
