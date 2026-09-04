#include "SunburstWidget.h"
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
#include <cmath>
#include <algorithm>

SunburstWidget::SunburstWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void SunburstWidget::setRootNode(DiskNode* rootNode) {
    m_dataRoot = rootNode;
    m_currentRoot = rootNode;
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    m_hoveredCenter = false;
    computeLayout();
    update();
    emit currentRootChanged(m_currentRoot);
}

void SunburstWidget::zoomIn(DiskNode* node) {
    if (!node || !node->isDirectory()) return;
    m_currentRoot = node;
    computeLayout();
    update();
    emit currentRootChanged(m_currentRoot);
}

void SunburstWidget::zoomOut() {
    if (!m_currentRoot || m_currentRoot == m_dataRoot) return;
    if (m_currentRoot->parent()) {
        m_currentRoot = m_currentRoot->parent();
        computeLayout();
        update();
        emit currentRootChanged(m_currentRoot);
    }
}

void SunburstWidget::selectNode(DiskNode* node) {
    if (m_selectedNode != node) {
        m_selectedNode = node;
        update();
    }
}

void SunburstWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    computeLayout();
}

void SunburstWidget::computeLayout() {
    m_slices.clear();
    if (!m_currentRoot || m_currentRoot->size() == 0) return;

    double w = width();
    double h = height();
    m_center = QPointF(w / 2.0, h / 2.0);

    double maxRadius = (std::min(w, h) / 2.0) - 20.0;
    if (maxRadius < 60.0) return;

    m_hubRadius = std::clamp(maxRadius * 0.28, 45.0, 110.0);
    double availableRadial = maxRadius - m_hubRadius;
    double ringWidth = availableRadial / m_maxDepth;

    buildSlices(m_currentRoot, 0.0, 360.0, m_hubRadius + 3.0, ringWidth, 0, m_maxDepth, m_center);
}

void SunburstWidget::buildSlices(DiskNode* parent, double startAngle, double spanAngle,
                                 double innerR, double ringWidth, int depth, int maxDepth,
                                 const QPointF& center) {
    if (!parent || depth >= maxDepth || spanAngle < 0.5) return;

    uint64_t totalSize = parent->size();
    if (totalSize == 0) return;

    double outerR = innerR + ringWidth - 2.0; // 2px radial gap between rings
    double currentAngle = startAngle;

    for (const auto& child : parent->children()) {
        if (child->size() == 0) continue;

        double childFraction = static_cast<double>(child->size()) / totalSize;
        double childSpan = spanAngle * childFraction;

        if (childSpan < 0.3) {
            currentAngle += childSpan;
            continue;
        }

        // Generate annular sector path
        QPainterPath path;
        QRectF outerRect(center.x() - outerR, center.y() - outerR, 2.0 * outerR, 2.0 * outerR);
        QRectF innerRect(center.x() - innerR, center.y() - innerR, 2.0 * innerR, 2.0 * innerR);

        path.arcMoveTo(outerRect, currentAngle);
        path.arcTo(outerRect, currentAngle, childSpan);
        path.arcTo(innerRect, currentAngle + childSpan, -childSpan);
        path.closeSubpath();

        SunburstSlice slice;
        slice.node = child.get();
        slice.innerRadius = innerR;
        slice.outerRadius = outerR;
        slice.startAngle = currentAngle;
        slice.spanAngle = childSpan;
        slice.depth = depth;
        slice.path = path;
        m_slices.push_back(slice);

        // Recurse into directory children
        if (child->isDirectory() && depth + 1 < maxDepth) {
            buildSlices(child.get(), currentAngle, childSpan,
                        outerR + 3.0, ringWidth, depth + 1, maxDepth, center);
        }

        currentAngle += childSpan;
    }
}

bool SunburstWidget::isCenterHub(const QPointF& pos) const {
    double dx = pos.x() - m_center.x();
    double dy = pos.y() - m_center.y();
    return (dx * dx + dy * dy) <= (m_hubRadius * m_hubRadius);
}

SunburstSlice* SunburstWidget::sliceAt(const QPointF& pos) {
    // Search in reverse so outermost/deeper rings are tested first
    for (auto it = m_slices.rbegin(); it != m_slices.rend(); ++it) {
        if (it->path.contains(pos)) {
            return &(*it);
        }
    }
    return nullptr;
}

void SunburstWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Deep Dark Background
    painter.fillRect(rect(), QColor(18, 21, 27));

    if (!m_currentRoot || m_slices.empty()) {
        painter.setPen(QColor(130, 140, 155));
        painter.drawText(rect(), Qt::AlignCenter,
                         m_currentRoot ? QStringLiteral("Folder is empty or contains 0-byte files")
                                       : QStringLiteral("No scan data available. Start a scan to view sunburst."));
        return;
    }

    // 1. Draw concentric annular slices
    for (const auto& slice : m_slices) {
        DiskNode* node = slice.node;
        bool isHovered = (node == m_hoveredNode);
        bool isSelected = (node == m_selectedNode);

        QColor baseColor;
        if (node->isDirectory()) {
            baseColor = QColor(60, 70, 85);
        } else {
            baseColor = DiskNode::getColorForExtension(node->extension());
        }

        // Slightly brighten or darken based on depth
        if (slice.depth > 0) {
            baseColor = baseColor.darker(100 + slice.depth * 12);
        }

        if (isHovered) {
            baseColor = baseColor.lighter(135);
        } else if (isSelected) {
            baseColor = baseColor.lighter(120);
        }

        painter.setBrush(baseColor);

        if (isSelected) {
            painter.setPen(QPen(QColor(255, 215, 0), 2.0)); // Gold selection
        } else if (isHovered) {
            painter.setPen(QPen(QColor(255, 255, 255), 1.5)); // White hover
        } else {
            painter.setPen(QPen(QColor(18, 21, 27, 240), 1.0)); // Clean dark gap
        }

        painter.drawPath(slice.path);
    }

    // 2. Draw Center Hub (DaisyDisk style circular core)
    QRectF hubRect(m_center.x() - m_hubRadius, m_center.y() - m_hubRadius,
                   2.0 * m_hubRadius, 2.0 * m_hubRadius);

    QRadialGradient hubGrad(m_center, m_hubRadius);
    if (m_hoveredCenter) {
        hubGrad.setColorAt(0.0, QColor(45, 55, 72));
        hubGrad.setColorAt(1.0, QColor(30, 38, 50));
    } else {
        hubGrad.setColorAt(0.0, QColor(32, 38, 48));
        hubGrad.setColorAt(1.0, QColor(22, 27, 34));
    }

    painter.setBrush(hubGrad);
    if (m_hoveredCenter) {
        painter.setPen(QPen(QColor(88, 166, 255), 2.0));
    } else {
        painter.setPen(QPen(QColor(52, 60, 72), 1.5));
    }
    painter.drawEllipse(hubRect);

    // Center Hub Text
    QRectF textRect = hubRect.adjusted(8, 8, -8, -8);
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(m_hubRadius > 70 ? 10 : 9);
    painter.setFont(titleFont);
    painter.setPen(QColor(240, 243, 246));

    QFontMetrics fm(titleFont);
    QString name = m_currentRoot->name().isEmpty() ? m_currentRoot->fullPath() : m_currentRoot->name();
    QString elidedName = fm.elidedText(name, Qt::ElideMiddle, static_cast<int>(textRect.width()));

    // Center text vertically
    double totalTextH = fm.height() * 2 + 10;
    double startY = m_center.y() - totalTextH / 2.0;

    QRectF nameRect(textRect.left(), startY, textRect.width(), fm.height());
    painter.drawText(nameRect, Qt::AlignCenter, elidedName);

    // Size text
    QFont sizeFont = font();
    sizeFont.setPointSize(m_hubRadius > 70 ? 9 : 8);
    painter.setFont(sizeFont);
    painter.setPen(QColor(88, 166, 255)); // Bright Blue
    QRectF sizeRect(textRect.left(), startY + fm.height() + 2, textRect.width(), fm.height());
    painter.drawText(sizeRect, Qt::AlignCenter, DiskNode::formatSize(m_currentRoot->size()));

    // If zoomed in, show "▲ Up" hint at bottom of center hub
    if (m_currentRoot != m_dataRoot && m_currentRoot->parent()) {
        QFont upFont = font();
        upFont.setPointSize(7);
        painter.setFont(upFont);
        painter.setPen(m_hoveredCenter ? QColor(255, 255, 255) : QColor(139, 148, 158));
        QRectF upRect(textRect.left(), startY + fm.height() * 2 + 4, textRect.width(), fm.height());
        painter.drawText(upRect, Qt::AlignCenter, QStringLiteral("▲ Click to Go Up"));
    }
}

void SunburstWidget::mouseMoveEvent(QMouseEvent* event) {
    QPointF pos = event->position();

    bool onCenter = isCenterHub(pos);
    if (onCenter != m_hoveredCenter) {
        m_hoveredCenter = onCenter;
        update();
        if (m_hoveredCenter && m_currentRoot != m_dataRoot && m_currentRoot->parent()) {
            QToolTip::showText(event->globalPosition().toPoint(),
                               QStringLiteral("<b>Click center to navigate up</b> to: ") + m_currentRoot->parent()->name(),
                               this);
            return;
        }
    }

    if (m_hoveredCenter) {
        if (m_hoveredNode) {
            m_hoveredNode = nullptr;
            update();
        }
        return;
    }

    SunburstSlice* slice = sliceAt(pos);
    DiskNode* newHover = slice ? slice->node : nullptr;

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

void SunburstWidget::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (m_hoveredNode || m_hoveredCenter) {
        m_hoveredNode = nullptr;
        m_hoveredCenter = false;
        update();
        QToolTip::hideText();
    }
}

void SunburstWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (isCenterHub(event->position())) {
            zoomOut();
            return;
        }

        SunburstSlice* slice = sliceAt(event->position());
        if (slice && slice->node) {
            m_selectedNode = slice->node;
            update();
            emit nodeSelected(m_selectedNode);
        }
    }
}

void SunburstWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        SunburstSlice* slice = sliceAt(event->position());
        if (slice && slice->node) {
            emit nodeDoubleClicked(slice->node);
            if (slice->node->isDirectory()) {
                zoomIn(slice->node);
            }
        }
    }
}

void SunburstWidget::contextMenuEvent(QContextMenuEvent* event) {
    SunburstSlice* slice = sliceAt(event->pos());
    if (!slice || !slice->node) return;

    DiskNode* node = slice->node;
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
