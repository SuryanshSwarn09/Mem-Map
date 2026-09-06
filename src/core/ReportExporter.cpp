#include "ReportExporter.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <map>
#include <vector>

static QString escapeCsv(const QString& str) {
    QString res = str;
    res.replace('"', "\"\"");
    return "\"" + res + "\"";
}

bool ReportExporter::exportToCsv(const DiskNode* root, const QString& filePath, QString* errorMsg) {
    if (!root) {
        if (errorMsg) *errorMsg = QStringLiteral("Root node is null.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    // Write UTF-8 BOM so Excel on Windows recognizes UTF-8 cleanly
    out << "\xEF\xBB\xBF";

    // Header
    out << "Path,Name,Type,SizeBytes,SizeFormatted,FileCount,FolderCount,Extension,Category\n";

    writeCsvNode(root, out);
    return true;
}

void ReportExporter::writeCsvNode(const DiskNode* node, QTextStream& out) {
    if (!node) return;

    QString type = node->isDirectory() ? QStringLiteral("Folder") : QStringLiteral("File");
    QString ext = node->isDirectory() ? QString() : node->extension();
    QString cat = node->isDirectory() ? QStringLiteral("Directory") : DiskNode::getFileCategory(ext);

    out << escapeCsv(node->fullPath()) << ","
        << escapeCsv(node->name()) << ","
        << escapeCsv(type) << ","
        << QString::number(node->size()) << ","
        << escapeCsv(DiskNode::formatSize(node->size())) << ","
        << QString::number(node->fileCount()) << ","
        << QString::number(node->dirCount()) << ","
        << escapeCsv(ext) << ","
        << escapeCsv(cat) << "\n";

    if (node->isDirectory()) {
        for (const auto& child : node->children()) {
            writeCsvNode(child.get(), out);
        }
    }
}

QJsonObject ReportExporter::nodeToJsonObject(const DiskNode* node) {
    QJsonObject obj;
    if (!node) return obj;

    obj[QStringLiteral("name")] = node->name();
    obj[QStringLiteral("path")] = node->fullPath();
    obj[QStringLiteral("isDir")] = node->isDirectory();
    obj[QStringLiteral("size")] = static_cast<qint64>(node->size());
    obj[QStringLiteral("files")] = static_cast<qint64>(node->fileCount());
    obj[QStringLiteral("folders")] = static_cast<qint64>(node->dirCount());

    if (!node->isDirectory()) {
        obj[QStringLiteral("extension")] = node->extension();
        obj[QStringLiteral("category")] = DiskNode::getFileCategory(node->extension());
    } else {
        QJsonArray childrenArr;
        for (const auto& child : node->children()) {
            childrenArr.append(nodeToJsonObject(child.get()));
        }
        obj[QStringLiteral("children")] = childrenArr;
    }

    return obj;
}

bool ReportExporter::exportToJson(const DiskNode* root, const QString& filePath, QString* errorMsg) {
    if (!root) {
        if (errorMsg) *errorMsg = QStringLiteral("Root node is null.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = file.errorString();
        return false;
    }

    QJsonObject rootObj;
    rootObj[QStringLiteral("generator")] = QStringLiteral("Mem-Map Storage Visualizer");
    rootObj[QStringLiteral("version")] = QStringLiteral("1.0.0");
    rootObj[QStringLiteral("exportDate")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObj[QStringLiteral("rootPath")] = root->fullPath();
    rootObj[QStringLiteral("totalSize")] = static_cast<qint64>(root->size());
    rootObj[QStringLiteral("totalFiles")] = static_cast<qint64>(root->fileCount());
    rootObj[QStringLiteral("totalFolders")] = static_cast<qint64>(root->dirCount());
    rootObj[QStringLiteral("tree")] = nodeToJsonObject(root);

    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

static void collectTopFiles(const DiskNode* node, std::vector<const DiskNode*>& list) {
    if (!node) return;
    if (!node->isDirectory()) {
        if (node->size() > 0) list.push_back(node);
        return;
    }
    for (const auto& child : node->children()) {
        collectTopFiles(child.get(), list);
    }
}

static void collectExtStats(const DiskNode* node, std::map<QString, std::pair<uint64_t, uint64_t>>& map) {
    if (!node) return;
    if (!node->isDirectory()) {
        QString ext = node->extension();
        map[ext].first += node->size();
        map[ext].second += 1;
        return;
    }
    for (const auto& child : node->children()) {
        collectExtStats(child.get(), map);
    }
}

bool ReportExporter::exportToHtml(const DiskNode* root, const QString& filePath, QString* errorMsg) {
    if (!root) {
        if (errorMsg) *errorMsg = QStringLiteral("Root node is null.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = file.errorString();
        return false;
    }

    QString html = generateHtmlReport(root);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << html;
    return true;
}

QString ReportExporter::generateHtmlReport(const DiskNode* root) {
    // Collect Top 50 Files
    std::vector<const DiskNode*> allFiles;
    collectTopFiles(root, allFiles);
    std::sort(allFiles.begin(), allFiles.end(), [](const DiskNode* a, const DiskNode* b) {
        return a->size() > b->size();
    });
    size_t topCount = std::min(size_t(50), allFiles.size());

    // Collect Extension Stats
    std::map<QString, std::pair<uint64_t, uint64_t>> extMap;
    collectExtStats(root, extMap);
    struct ExtItem { QString ext; uint64_t bytes; uint64_t count; };
    std::vector<ExtItem> extList;
    for (const auto& p : extMap) {
        extList.push_back(ExtItem{p.first, p.second.first, p.second.second});
    }
    std::sort(extList.begin(), extList.end(), [](const ExtItem& a, const ExtItem& b) {
        return a.bytes > b.bytes;
    });

    // Build JSON data for the inlined canvas interactive treemap
    QJsonDocument treeDoc(nodeToJsonObject(root));
    QString treeJson = QString::fromUtf8(treeDoc.toJson(QJsonDocument::Compact));

    QString html;
    QTextStream s(&html);

    s << "<!DOCTYPE html>\n"
      << "<html lang=\"en\">\n"
      << "<head>\n"
      << "  <meta charset=\"UTF-8\">\n"
      << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
      << "  <title>Mem-Map Storage Report - " << root->name().toHtmlEscaped() << "</title>\n"
      << "  <style>\n"
      << "    * { box-sizing: border-box; margin: 0; padding: 0; }\n"
      << "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #0D1117; color: #C9D1D9; line-height: 1.5; padding: 24px; }\n"
      << "    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #30363D; padding-bottom: 16px; margin-bottom: 24px; }\n"
      << "    .title-area h1 { font-size: 24px; color: #58A6FF; margin-bottom: 4px; }\n"
      << "    .title-area p { color: #8B949E; font-size: 14px; }\n"
      << "    .kpi-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px; margin-bottom: 24px; }\n"
      << "    .kpi-card { background-color: #161B22; border: 1px solid #30363D; border-radius: 8px; padding: 16px; }\n"
      << "    .kpi-card .label { font-size: 12px; font-weight: 600; color: #8B949E; text-transform: uppercase; margin-bottom: 6px; }\n"
      << "    .kpi-card .value { font-size: 22px; font-weight: 700; color: #FFFFFF; }\n"
      << "    .section { background-color: #161B22; border: 1px solid #30363D; border-radius: 8px; padding: 20px; margin-bottom: 24px; }\n"
      << "    .section h2 { font-size: 18px; color: #FFFFFF; margin-bottom: 14px; display: flex; justify-content: space-between; align-items: center; }\n"
      << "    .canvas-container { position: relative; width: 100%; height: 500px; background-color: #12151B; border: 1px solid #21262D; border-radius: 6px; overflow: hidden; }\n"
      << "    canvas { width: 100%; height: 100%; display: block; cursor: pointer; }\n"
      << "    .tooltip { position: absolute; display: none; background-color: #1F242C; border: 1px solid #58A6FF; border-radius: 6px; padding: 8px 12px; font-size: 12px; pointer-events: none; z-index: 1000; box-shadow: 0 4px 12px rgba(0,0,0,0.5); }\n"
      << "    .nav-bar { display: flex; gap: 8px; align-items: center; margin-bottom: 12px; font-size: 13px; }\n"
      << "    .nav-btn { background-color: #21262D; border: 1px solid #30363D; color: #C9D1D9; padding: 4px 10px; border-radius: 4px; cursor: pointer; font-weight: 600; }\n"
      << "    .nav-btn:hover { background-color: #30363D; border-color: #58A6FF; color: #FFF; }\n"
      << "    table { width: 100%; border-collapse: collapse; font-size: 13px; text-align: left; }\n"
      << "    th { background-color: #21262D; color: #8B949E; padding: 10px 12px; font-weight: 600; border-bottom: 1px solid #30363D; }\n"
      << "    td { padding: 8px 12px; border-bottom: 1px solid #21262D; }\n"
      << "    tr:hover td { background-color: #1F242C; }\n"
      << "    .bar-bg { width: 100px; height: 8px; background-color: #30363D; border-radius: 4px; overflow: hidden; display: inline-block; vertical-align: middle; margin-right: 8px; }\n"
      << "    .bar-fill { height: 100%; background: linear-gradient(90deg, #1F6FEB, #58A6FF); border-radius: 4px; }\n"
      << "    .badge { display: inline-block; padding: 2px 6px; border-radius: 4px; font-size: 11px; font-weight: 600; }\n"
      << "  </style>\n"
      << "</head>\n"
      << "<body>\n"
      << "  <div class=\"header\">\n"
      << "    <div class=\"title-area\">\n"
      << "      <h1>Mem-Map Storage Report</h1>\n"
      << "      <p>Target: <code>" << root->fullPath().toHtmlEscaped() << "</code> &bull; Generated: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "</p>\n"
      << "    </div>\n"
      << "  </div>\n"
      << "  <div class=\"kpi-grid\">\n"
      << "    <div class=\"kpi-card\"><div class=\"label\">Total Space</div><div class=\"value\" style=\"color:#58A6FF;\">" << DiskNode::formatSize(root->size()) << "</div></div>\n"
      << "    <div class=\"kpi-card\"><div class=\"label\">Total Files</div><div class=\"value\" style=\"color:#7EE787;\">" << QString::number(root->fileCount()) << "</div></div>\n"
      << "    <div class=\"kpi-card\"><div class=\"label\">Subdirectories</div><div class=\"value\" style=\"color:#FFA657;\">" << QString::number(root->dirCount()) << "</div></div>\n"
      << "    <div class=\"kpi-card\"><div class=\"label\">Top Extension</div><div class=\"value\" style=\"color:#D2A8FF;\">" << (extList.empty() ? QStringLiteral("N/A") : ("." + extList[0].ext)) << "</div></div>\n"
      << "  </div>\n"
      << "  <div class=\"section\">\n"
      << "    <h2>Interactive Storage Treemap <span style=\"font-size:12px; font-weight:normal; color:#8B949E;\">Click a folder to drill down &bull; Click 'Up' to zoom out</span></h2>\n"
      << "    <div class=\"nav-bar\">\n"
      << "      <button class=\"nav-btn\" id=\"btnUp\" onclick=\"zoomOut()\">&#x25B2; Up</button>\n"
      << "      <button class=\"nav-btn\" id=\"btnReset\" onclick=\"resetZoom()\">&#x1F3E0; Root</button>\n"
      << "      <span id=\"currentPathLabel\" style=\"color:#58A6FF; font-weight:600;\">" << root->name().toHtmlEscaped() << "</span>\n"
      << "    </div>\n"
      << "    <div class=\"canvas-container\" id=\"container\">\n"
      << "      <canvas id=\"treemapCanvas\"></canvas>\n"
      << "      <div class=\"tooltip\" id=\"tooltip\"></div>\n"
      << "    </div>\n"
      << "  </div>\n"
      << "  <div class=\"section\">\n"
      << "    <h2>Storage by File Extension</h2>\n"
      << "    <table>\n"
      << "      <tr><th>Extension</th><th>Category</th><th>Total Size</th><th>Share %</th><th>Files</th></tr>\n";

    double totalBytes = static_cast<double>(root->size());
    size_t extShowCount = std::min(size_t(15), extList.size());
    for (size_t i = 0; i < extShowCount; ++i) {
        const auto& item = extList[i];
        double pct = (totalBytes > 0) ? (static_cast<double>(item.bytes) / totalBytes * 100.0) : 0.0;
        QString cat = DiskNode::getFileCategory(item.ext);
        s << "      <tr>\n"
          << "        <td><b>" << (item.ext == "(none)" ? "[No Ext]" : ("." + item.ext.toUpper())) << "</b></td>\n"
          << "        <td>" << cat << "</td>\n"
          << "        <td>" << DiskNode::formatSize(item.bytes) << "</td>\n"
          << "        <td><div class=\"bar-bg\"><div class=\"bar-fill\" style=\"width:" << QString::number(pct, 'f', 1) << "%\"></div></div>" << QString::number(pct, 'f', 1) << "%</td>\n"
          << "        <td>" << QString::number(item.count) << "</td>\n"
          << "      </tr>\n";
    }

    s << "    </table>\n"
      << "  </div>\n"
      << "  <div class=\"section\">\n"
      << "    <h2>Top 50 Largest Files</h2>\n"
      << "    <table>\n"
      << "      <tr><th>#</th><th>File Name</th><th>Size</th><th>Type</th><th>Folder Path</th></tr>\n";

    for (size_t i = 0; i < topCount; ++i) {
        const auto* fn = allFiles[i];
        QString ext = fn->extension().toUpper();
        s << "      <tr>\n"
          << "        <td style=\"color:#8B949E;\">" << QString::number(i + 1) << "</td>\n"
          << "        <td><b>" << fn->name().toHtmlEscaped() << "</b></td>\n"
          << "        <td style=\"color:#58A6FF; font-weight:600;\">" << DiskNode::formatSize(fn->size()) << "</td>\n"
          << "        <td>" << ext << "</td>\n"
          << "        <td style=\"color:#8B949E;\">" << (fn->parent() ? fn->parent()->fullPath().toHtmlEscaped() : "") << "</td>\n"
          << "      </tr>\n";
    }

    s << "    </table>\n"
      << "  </div>\n"
      << "  <script>\n"
      << "    const rawData = " << treeJson << ";\n"
      << "    let currentRoot = rawData;\n"
      << "    let rootHistory = [];\n"
      << "    const canvas = document.getElementById('treemapCanvas');\n"
      << "    const ctx = canvas.getContext('2d');\n"
      << "    const tooltip = document.getElementById('tooltip');\n"
      << "    const container = document.getElementById('container');\n"
      << "    let tiles = [];\n"
      << "    let hoveredTile = null;\n"
      << "    const catColors = {\n"
      << "      'Video': '#9C27B0', 'Audio': '#00BCD4', 'Image': '#FF9800',\n"
      << "      'Document': '#2196F3', 'Archive': '#FFC107', 'Executable': '#4CAF50',\n"
      << "      'Development': '#E91E63', 'Other': '#78909C', 'Directory': '#343D4D'\n"
      << "    };\n"
      << "    function resizeCanvas() {\n"
      << "      const rect = container.getBoundingClientRect();\n"
      << "      canvas.width = rect.width;\n"
      << "      canvas.height = rect.height;\n"
      << "      renderTreemap();\n"
      << "    }\n"
      << "    window.addEventListener('resize', resizeCanvas);\n"
      << "    function formatBytes(bytes) {\n"
      << "      if (bytes >= 1e12) return (bytes / 1e12).toFixed(2) + ' TB';\n"
      << "      if (bytes >= 1e9) return (bytes / 1e9).toFixed(2) + ' GB';\n"
      << "      if (bytes >= 1e6) return (bytes / 1e6).toFixed(1) + ' MB';\n"
      << "      if (bytes >= 1e3) return (bytes / 1e3).toFixed(1) + ' KB';\n"
      << "      return bytes + ' B';\n"
      << "    }\n"
      << "    function worst(row, w) {\n"
      << "      if (!row.length || w <= 0) return 1e9;\n"
      << "      let sum = row.reduce((a, b) => a + b, 0);\n"
      << "      let max = Math.max(...row), min = Math.min(...row);\n"
      << "      let s2 = sum * sum, w2 = w * w;\n"
      << "      return Math.max((w2 * max) / s2, s2 / (w2 * min));\n"
      << "    }\n"
      << "    function squarify(items, rect, depth, maxDepth) {\n"
      << "      let valid = items.filter(c => c.size > 0);\n"
      << "      let total = valid.reduce((acc, c) => acc + c.size, 0);\n"
      << "      if (!valid.length || total <= 0) return;\n"
      << "      let totalArea = rect.w * rect.h;\n"
      << "      let areas = valid.map(c => (c.size / total) * totalArea);\n"
      << "      let x = rect.x, y = rect.y, w = rect.w, h = rect.h;\n"
      << "      let idx = 0;\n"
      << "      while (idx < valid.length && w >= 2 && h >= 2) {\n"
      << "        let isHoriz = w >= h;\n"
      << "        let side = isHoriz ? h : w;\n"
      << "        let row = [areas[idx]];\n"
      << "        let rowItems = [valid[idx]];\n"
      << "        let curWorst = worst(row, side);\n"
      << "        let next = idx + 1;\n"
      << "        while (next < valid.length) {\n"
      << "          let testRow = [...row, areas[next]];\n"
      << "          let testWorst = worst(testRow, side);\n"
      << "          if (testWorst <= curWorst) {\n"
      << "            row.push(areas[next]);\n"
      << "            rowItems.push(valid[next]);\n"
      << "            curWorst = testWorst;\n"
      << "            next++;\n"
      << "          } else break;\n"
      << "        }\n"
      << "        let rowSum = row.reduce((a, b) => a + b, 0);\n"
      << "        let thickness = side > 0 ? (rowSum / side) : 0;\n"
      << "        let offset = 0;\n"
      << "        for (let i = 0; i < rowItems.length; i++) {\n"
      << "          let item = rowItems[i];\n"
      << "          let len = thickness > 0 ? (row[i] / thickness) : 0;\n"
      << "          let tr = isHoriz ? {x: x, y: y + offset, w: thickness, h: len} : {x: x + offset, y: y, w: len, h: thickness};\n"
      << "          offset += len;\n"
      << "          if (tr.w >= 2 && tr.h >= 2) {\n"
      << "            if (item.isDir && depth < maxDepth && tr.w >= 24 && tr.h >= 24 && item.children) {\n"
      << "              squarify(item.children, {x: tr.x + 1, y: tr.y + 1, w: tr.w - 2, h: tr.h - 2}, depth + 1, maxDepth);\n"
      << "            } else {\n"
      << "              tiles.push({node: item, rect: tr});\n"
      << "            }\n"
      << "          }\n"
      << "        }\n"
      << "        if (isHoriz) { x += thickness; w -= thickness; }\n"
      << "        else { y += thickness; h -= thickness; }\n"
      << "        idx = next;\n"
      << "      }\n"
      << "    }\n"
      << "    function renderTreemap() {\n"
      << "      tiles = [];\n"
      << "      ctx.fillStyle = '#12151B';\n"
      << "      ctx.fillRect(0, 0, canvas.width, canvas.height);\n"
      << "      if (!currentRoot || !currentRoot.children) return;\n"
      << "      squarify(currentRoot.children, {x: 2, y: 2, w: canvas.width - 4, h: canvas.height - 4}, 0, 2);\n"
      << "      for (let t of tiles) {\n"
      << "        let r = t.rect, n = t.node;\n"
      << "        let baseCol = n.isDir ? catColors['Directory'] : (catColors[n.category] || catColors['Other']);\n"
      << "        ctx.fillStyle = (t === hoveredTile) ? '#58A6FF' : baseCol;\n"
      << "        ctx.fillRect(r.x, r.y, r.w, r.h);\n"
      << "        ctx.strokeStyle = '#0D1117';\n"
      << "        ctx.lineWidth = 1;\n"
      << "        ctx.strokeRect(r.x, r.y, r.w, r.h);\n"
      << "        if (r.w >= 45 && r.h >= 22) {\n"
      << "          ctx.fillStyle = '#FFFFFF';\n"
      << "          ctx.font = '11px Segoe UI, sans-serif';\n"
      << "          ctx.fillText(n.name.substring(0, Math.floor(r.w / 7)), r.x + 4, r.y + 14);\n"
      << "        }\n"
      << "      }\n"
      << "    }\n"
      << "    canvas.addEventListener('mousemove', (e) => {\n"
      << "      let rect = canvas.getBoundingClientRect();\n"
      << "      let mx = e.clientX - rect.left, my = e.clientY - rect.top;\n"
      << "      hoveredTile = null;\n"
      << "      for (let i = tiles.length - 1; i >= 0; i--) {\n"
      << "        let t = tiles[i], r = t.rect;\n"
      << "        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {\n"
      << "          hoveredTile = t;\n"
      << "          break;\n"
      << "        }\n"
      << "      }\n"
      << "      if (hoveredTile) {\n"
      << "        let n = hoveredTile.node;\n"
      << "        let pct = ((n.size / currentRoot.size) * 100).toFixed(1);\n"
      << "        tooltip.innerHTML = '<b>' + n.name + '</b><br>Size: ' + formatBytes(n.size) + ' (' + pct + '%)<br>Type: ' + (n.isDir ? 'Folder' : n.extension.toUpperCase()) + '<br>' + n.path;\n"
      << "        tooltip.style.left = (mx + 15) + 'px';\n"
      << "        tooltip.style.top = (my + 15) + 'px';\n"
      << "        tooltip.style.display = 'block';\n"
      << "      } else {\n"
      << "        tooltip.style.display = 'none';\n"
      << "      }\n"
      << "      renderTreemap();\n"
      << "    });\n"
      << "    canvas.addEventListener('mouseleave', () => { hoveredTile = null; tooltip.style.display = 'none'; renderTreemap(); });\n"
      << "    canvas.addEventListener('click', () => {\n"
      << "      if (hoveredTile && hoveredTile.node.isDir) {\n"
      << "        rootHistory.push(currentRoot);\n"
      << "        currentRoot = hoveredTile.node;\n"
      << "        document.getElementById('currentPathLabel').innerText = currentRoot.path;\n"
      << "        renderTreemap();\n"
      << "      }\n"
      << "    });\n"
      << "    function zoomOut() {\n"
      << "      if (rootHistory.length > 0) {\n"
      << "        currentRoot = rootHistory.pop();\n"
      << "        document.getElementById('currentPathLabel').innerText = currentRoot.path;\n"
      << "        renderTreemap();\n"
      << "      }\n"
      << "    }\n"
      << "    function resetZoom() {\n"
      << "      currentRoot = rawData;\n"
      << "      rootHistory = [];\n"
      << "      document.getElementById('currentPathLabel').innerText = currentRoot.path;\n"
      << "      renderTreemap();\n"
      << "    }\n"
      << "    setTimeout(resizeCanvas, 50);\n"
      << "  </script>\n"
      << "</body>\n"
      << "</html>\n";

    return html;
}
