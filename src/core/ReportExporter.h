#pragma once

#include <QString>
#include <QTextStream>
#include <QJsonObject>
#include "DiskNode.h"

class ReportExporter {
public:
    static bool exportToCsv(const DiskNode* root, const QString& filePath, QString* errorMsg = nullptr);
    static bool exportToJson(const DiskNode* root, const QString& filePath, QString* errorMsg = nullptr);
    static bool exportToHtml(const DiskNode* root, const QString& filePath, QString* errorMsg = nullptr);

private:
    static void writeCsvNode(const DiskNode* node, QTextStream& out);
    static QJsonObject nodeToJsonObject(const DiskNode* node);
    static QString generateHtmlReport(const DiskNode* root);
};
