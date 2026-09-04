#include "ScannerEngine.h"
#include <filesystem>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

ScannerEngine::ScannerEngine(QObject* parent)
    : QThread(parent)
{
    qRegisterMetaType<std::shared_ptr<DiskNode>>("std::shared_ptr<DiskNode>");
}

ScannerEngine::~ScannerEngine() {
    cancelScan();
    wait();
}

void ScannerEngine::startScan(const QString& rootPath) {
    if (m_isScanning.load()) {
        cancelScan();
        wait();
    }

    m_targetPath = QDir::toNativeSeparators(rootPath);
    m_cancelRequested.store(false);
    m_isScanning.store(true);
    start();
}

void ScannerEngine::cancelScan() {
    m_cancelRequested.store(true);
}

bool ScannerEngine::isReparsePointOrSymlink(const QString& path) const {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()));
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
        return true;
    }
#endif
    std::error_code ec;
    auto status = fs::symlink_status(path.toStdWString(), ec);
    if (!ec && fs::is_symlink(status)) {
        return true;
    }
    return false;
}

std::unique_ptr<DiskNode> ScannerEngine::scanDirectory(const QString& dirPath, DiskNode* parentNode) {
    if (m_cancelRequested.load()) {
        return nullptr;
    }

    QFileInfo info(dirPath);
    QString name = info.fileName();
    if (name.isEmpty()) {
        name = dirPath; // Root drives like C:/ or C:
    }

    auto dirNode = std::make_unique<DiskNode>(name, dirPath, true, parentNode);

    // Throttle progress updates: every 60ms
    qint64 now = m_timer.elapsed();
    if (now - m_lastProgressTime > 60) {
        m_lastProgressTime = now;
        emit scanProgress(m_totalFiles, m_totalBytes, dirPath);
    }

    std::error_code ec;
    fs::path p(dirPath.toStdWString());
    
    // Check if accessible
    if (!fs::exists(p, ec) || ec) {
        return dirNode;
    }

    auto options = fs::directory_options::skip_permission_denied;
    fs::directory_iterator it(p, options, ec);
    fs::directory_iterator endIt;

    if (ec) {
        return dirNode;
    }

    while (it != endIt && !m_cancelRequested.load()) {
        try {
            const auto& entry = *it;
            std::error_code entryEc;
            auto status = entry.symlink_status(entryEc);

            if (!entryEc) {
                QString childPath = QString::fromStdWString(entry.path().wstring());
                QString childName = QString::fromStdWString(entry.path().filename().wstring());

                if (fs::is_directory(status)) {
                    // Check for symlinks/reparse points to prevent cycles
                    if (isReparsePointOrSymlink(childPath)) {
                        // Include as a node but don't recursively traverse into junctions
                        auto junctionNode = std::make_unique<DiskNode>(childName + QStringLiteral(" [Link]"), childPath, true, dirNode.get());
                        dirNode->addChild(std::move(junctionNode));
                    } else {
                        auto subDirNode = scanDirectory(childPath, dirNode.get());
                        if (subDirNode) {
                            dirNode->addChild(std::move(subDirNode));
                        }
                    }
                } else if (fs::is_regular_file(status)) {
                    uint64_t fileSize = 0;
                    std::error_code sizeEc;
                    fileSize = entry.file_size(sizeEc);
                    if (sizeEc) {
                        fileSize = 0;
                    }

                    auto fileNode = std::make_unique<DiskNode>(childName, childPath, false, dirNode.get());
                    fileNode->setSize(fileSize);
                    dirNode->addChild(std::move(fileNode));

                    m_totalFiles++;
                    m_totalBytes += fileSize;
                }
            }
        } catch (...) {
            // Ignore single file error, continue scanning directory
        }

        it.increment(ec);
        if (ec) {
            break;
        }
    }

    return dirNode;
}

void ScannerEngine::run() {
    emit scanStarted(m_targetPath);

    m_totalFiles = 0;
    m_totalBytes = 0;
    m_lastProgressTime = 0;
    m_timer.restart();

    std::unique_ptr<DiskNode> rootNode;

    try {
        rootNode = scanDirectory(m_targetPath, nullptr);
    } catch (const std::exception& e) {
        emit scanError(QString::fromLocal8Bit(e.what()));
    }

    bool wasCancelled = m_cancelRequested.load();

    if (rootNode) {
        // Bottom-up aggregation
        rootNode->calculateBottomUpSizes();
        rootNode->sortChildrenBySize();
    }

    qint64 elapsed = m_timer.elapsed();
    m_isScanning.store(false);

    std::shared_ptr<DiskNode> sharedRoot = std::move(rootNode);
    emit scanFinished(sharedRoot, elapsed, wasCancelled);
}
