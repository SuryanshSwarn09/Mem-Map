#pragma once

#include <QThread>
#include <QString>
#include <QElapsedTimer>
#include <atomic>
#include <memory>
#include "DiskNode.h"

class ScannerEngine : public QThread {
    Q_OBJECT

public:
    explicit ScannerEngine(QObject* parent = nullptr);
    ~ScannerEngine() override;

    void startScan(const QString& rootPath);
    void cancelScan();
    bool isScanning() const { return m_isScanning.load(); }

signals:
    void scanStarted(const QString& rootPath);
    void scanProgress(quint64 filesScanned, quint64 bytesScanned, const QString& currentFolder);
    void scanFinished(std::shared_ptr<DiskNode> rootNode, qint64 elapsedMs, bool wasCancelled);
    void scanError(const QString& message);

protected:
    void run() override;

private:
    std::unique_ptr<DiskNode> scanDirectory(const QString& dirPath, DiskNode* parentNode);
    bool isReparsePointOrSymlink(const QString& path) const;

    QString m_targetPath;
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<bool> m_isScanning{false};

    quint64 m_totalFiles{0};
    quint64 m_totalBytes{0};
    QElapsedTimer m_timer;
    qint64 m_lastProgressTime{0};
};
