#include "TreemapWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QToolTip>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

TreemapWidget::TreemapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void TreemapWidget::setRootNode(DiskNode* rootNode) {
    m_dataRoot = rootNode;
    m_currentRoot = rootNode;
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    relayout();
    update();
    emit currentRootChanged(m_currentRoot);
}

void TreemapWidget::zoomIn(DiskNode* node) {
    if (!node || !node->isDirectory()) return;
    m_currentRoot = node;
    relayout();
    update();
    emit currentRootChanged(m_currentRoot);
}

void TreemapWidget::zoomOut() {
    if (!m_currentRoot || m_currentRoot == m_dataRoot) return;
    if (m_currentRoot->parent()) {
        m_currentRoot = m_currentRoot->parent();
        relayout();
        update();
        emit currentRootChanged(m_currentRoot);
    }
}

void TreemapWidget::selectNode(DiskNode* node) {
    if (m_selectedNode != node) {
        m_selectedNode = node;
        update();
    }
}

void TreemapWidget::relayout() {
    if (!m_currentRoot) {
        m_tiles.clear();
        return;
    }
    QRectF bounds = rect().adjusted(2, 2, -2, -2);
    m_tiles = TreemapLayout::compute(m_currentRoot, bounds, m_maxDepth);
}

TreemapTile* TreemapWidget::tileAt(const QPointF& pos) {
    // Search in reverse so deeper tiles (rendered on top) are hit first
    for (auto it = m_tiles.rbegin(); it != m_tiles.rend(); ++it) {
        if (it->rect.contains(pos)) {
            return &(*it);
        }
    }
    return nullptr;
}

void TreemapWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    relayout();
}

void TreemapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Background
    painter.fillRect(rect(), QColor(24, 25, 29));

    if (!m_currentRoot || m_tiles.empty()) {
        painter.setPen(QColor(130, 140, 155));
        painter.drawText(rect(), Qt::AlignCenter, 
                         m_currentRoot ? QStringLiteral("Folder is empty or contains 0-byte files")
                                       : QStringLiteral("No scan data available. Start a scan to view treemap."));
        return;
    }

    QFont labelFont = font();
    labelFont.setPointSize(8);
    painter.setFont(labelFont);

    for (const auto& tile : m_tiles) {
        DiskNode* node = tile.node;
        QRectF r = tile.rect;

        // Base color
        QColor baseColor;
        if (node->isDirectory()) {
            baseColor = QColor(48, 55, 68);
        } else {
            baseColor = DiskNode::getColorForExtension(node->extension());
        }

        bool isHovered = (node == m_hoveredNode);
        bool isSelected = (node == m_selectedNode);

        // Cushion gradient effect
        QLinearGradient grad(r.topLeft(), r.bottomRight());
        if (isHovered) {
            grad.setColorAt(0.0, baseColor.lighter(135));
            grad.setColorAt(1.0, baseColor.lighter(105));
        } else if (isSelected) {
            grad.setColorAt(0.0, baseColor.lighter(120));
            grad.setColorAt(1.0, baseColor);
        } else {
            grad.setColorAt(0.0, baseColor.lighter(115));
            grad.setColorAt(0.8, baseColor);
            grad.setColorAt(1.0, baseColor.darker(125));
        }

        painter.setBrush(grad);

        // Border styling
        if (isSelected) {
            painter.setPen(QPen(QColor(255, 215, 0), 2.0)); // Bright Gold for selection
        } else if (isHovered) {
            painter.setPen(QPen(QColor(255, 255, 255), 1.5)); // White for hover
        } else {
            painter.setPen(QPen(QColor(18, 19, 23, 220), 0.8)); // Dark subtle separator
        }

        painter.drawRect(r);

        // Text label if space permits
        if (r.width() >= 48.0 && r.height() >= 24.0) {
            QRectF textRect = r.adjusted(3, 2, -3, -2);
            painter.setPen(QColor(240, 243, 246));

            QString displayText = node->name();
            QFontMetrics fm(labelFont);
            QString elidedName = fm.elidedText(displayText, Qt::ElideMiddle, static_cast<int>(textRect.width()));

            painter.drawText(textRect, Qt::AlignTop | Qt::AlignLeft, elidedName);

            if (r.height() >= 36.0) {
                QRectF sizeRect = textRect.adjusted(0, fm.height(), 0, 0);
                painter.setPen(QColor(200, 210, 220, 200));
                painter.drawText(sizeRect, Qt::AlignTop | Qt::AlignLeft, DiskNode::formatSize(node->size()));
            }
        }
    }
}

void TreemapWidget::mouseMoveEvent(QMouseEvent* event) {
    QPointF pos = event->position();
    TreemapTile* tile = tileAt(pos);

    DiskNode* newHover = tile ? tile->node : nullptr;
    if (newHover != m_hoveredNode) {
        m_hoveredNode = newHover;
        update();

        if (m_hoveredNode) {
            double percent = 0.0;
            if (m_currentRoot && m_currentRoot->size() > 0) {
                percent = (static_cast<double>(m_hoveredNode->size()) / m_currentRoot->size()) * 100.0;
            }

            QString tip = QStringLiteral(
                "<table style='font-family:Segoe UI, sans-serif; font-size:12px; color:#ffffff;'>"
                "<tr><td><b>Name:</b></td><td style='padding-left:8px;'>%1</td></tr>"
                "<tr><td><b>Size:</b></td><td style='padding-left:8px;'>%2 (%3%)</td></tr>"
                "<tr><td><b>Type:</b></td><td style='padding-left:8px;'>%4</td></tr>"
                "<tr><td><b>Path:</b></td><td style='padding-left:8px;'>%5</td></tr>"
                "</table>"
            ).arg(m_hoveredNode->name().toHtmlEscaped(),
                 DiskNode::formatSize(m_hoveredNode->size()),
                 QString::number(percent, 'f', 1),
                 m_hoveredNode->isDirectory() ? QStringLiteral("Folder") : m_hoveredNode->extension().toUpper(),
                 m_hoveredNode->fullPath().toHtmlEscaped());

            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
        } else {
            QToolTip::hideText();
        }
    }
}

void TreemapWidget::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (m_hoveredNode) {
        m_hoveredNode = nullptr;
        update();
        QToolTip::hideText();
    }
}

void TreemapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        TreemapTile* tile = tileAt(event->position());
        if (tile) {
            m_selectedNode = tile->node;
            update();
            emit nodeSelected(m_selectedNode);
        }
    }
}

void TreemapWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        TreemapTile* tile = tileAt(event->position());
        if (tile && tile->node) {
            emit nodeDoubleClicked(tile->node);
            if (tile->node->isDirectory()) {
                zoomIn(tile->node);
            }
        }
    }
}

void TreemapWidget::contextMenuEvent(QContextMenuEvent* event) {
    TreemapTile* tile = tileAt(event->pos());
    if (!tile || !tile->node) return;

    DiskNode* node = tile->node;
    QMenu menu(this);

    QAction* actOpenFolder = menu.addAction(QStringLiteral("Open in File Explorer"));
    QAction* actCopyPath = menu.addAction(QStringLiteral("Copy Full Path"));
    QAction* actZoomIn = nullptr;

    if (node->isDirectory()) {
        menu.addSeparator();
        actZoomIn = menu.addAction(QStringLiteral("Zoom into this folder"));
    }

    QAction* selected = menu.exec(event->globalPos());
    if (selected == actOpenFolder) {
#ifdef _WIN32
        QString path = QDir::toNativeSeparators(node->fullPath());
        QString param = QStringLiteral("/select,\"%1\"").arg(path);
        QProcess::startDetached(QStringLiteral("explorer.exe"), {param});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(node->fullPath()));
#endif
    } else if (selected == actCopyPath) {
        QGuiApplication::clipboard()->setText(node->fullPath());
    } else if (actZoomIn && selected == actZoomIn) {
        zoomIn(node);
    }
}
