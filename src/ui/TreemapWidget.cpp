#include "TreemapWidget.h"
#include "ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
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
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QDateTime>

TreemapWidget::TreemapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setupAnimation();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](ThemePreset) {
        update();
    });
}

void TreemapWidget::setRootNode(DiskNode* rootNode) {
    if (m_zoomAnim && m_zoomAnim->state() == QAbstractAnimation::Running) {
        m_zoomAnim->stop();
    }
    m_isAnimating = false;
    m_prevPixmap = QPixmap();
    m_nextPixmap = QPixmap();

    m_dataRoot = rootNode;
    m_currentRoot = rootNode;
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    relayout();
    update();
    emit currentRootChanged(m_currentRoot);
}

void TreemapWidget::setColorMode(ColorMode mode) {
    if (m_colorMode != mode) {
        m_colorMode = mode;
        update();
    }
}

void TreemapWidget::zoomIn(DiskNode* node) {
    if (!node || !node->isDirectory() || node == m_currentRoot) return;

    if (width() <= 0 || height() <= 0) {
        m_currentRoot = node;
        m_selectedNode = nullptr;
        m_hoveredNode = nullptr;
        relayout();
        update();
        emit currentRootChanged(m_currentRoot);
        return;
    }

    if (m_zoomAnim->state() == QAbstractAnimation::Running) {
        m_zoomAnim->stop();
    }

    QRectF tileRect = findTileRectForNode(node);
    if (tileRect.isEmpty()) {
        tileRect = QRectF(width() * 0.25, height() * 0.25, width() * 0.5, height() * 0.5);
    }

    m_prevPixmap = captureCurrentView();
    m_targetTileRect = tileRect;
    m_isZoomIn = true;

    m_currentRoot = node;
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    relayout();

    m_nextPixmap = captureCurrentView();

    m_isAnimating = true;
    m_animProgress = 0.0;
    m_zoomAnim->start();

    emit currentRootChanged(m_currentRoot);
}

void TreemapWidget::zoomOut() {
    if (!m_currentRoot || m_currentRoot == m_dataRoot) return;
    DiskNode* parentNode = m_currentRoot->parent();
    if (!parentNode) return;

    if (width() <= 0 || height() <= 0) {
        m_currentRoot = parentNode;
        m_selectedNode = nullptr;
        m_hoveredNode = nullptr;
        relayout();
        update();
        emit currentRootChanged(m_currentRoot);
        return;
    }

    if (m_zoomAnim->state() == QAbstractAnimation::Running) {
        m_zoomAnim->stop();
    }

    DiskNode* exitingChild = m_currentRoot;
    m_prevPixmap = captureCurrentView();

    m_currentRoot = parentNode;
    m_selectedNode = nullptr;
    m_hoveredNode = nullptr;
    relayout();

    QRectF destRect = findTileRectForNode(exitingChild);
    if (destRect.isEmpty()) {
        destRect = QRectF(width() * 0.25, height() * 0.25, width() * 0.5, height() * 0.5);
    }
    m_targetTileRect = destRect;
    m_isZoomIn = false;

    m_nextPixmap = captureCurrentView();

    m_isAnimating = true;
    m_animProgress = 0.0;
    m_zoomAnim->start();

    emit currentRootChanged(m_currentRoot);
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
    if (m_zoomAnim && m_zoomAnim->state() == QAbstractAnimation::Running) {
        m_zoomAnim->stop();
    }
    m_isAnimating = false;
    m_prevPixmap = QPixmap();
    m_nextPixmap = QPixmap();
    relayout();
}

void TreemapWidget::setupAnimation() {
    m_zoomAnim = new QVariantAnimation(this);
    m_zoomAnim->setDuration(280);
    m_zoomAnim->setStartValue(0.0);
    m_zoomAnim->setEndValue(1.0);
    m_zoomAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_zoomAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_animProgress = value.toReal();
        update();
    });
    connect(m_zoomAnim, &QVariantAnimation::finished, this, [this]() {
        m_isAnimating = false;
        m_prevPixmap = QPixmap();
        m_nextPixmap = QPixmap();
        update();
    });
}

QPixmap TreemapWidget::captureCurrentView() {
    if (width() <= 0 || height() <= 0) return QPixmap();
    QPixmap pixmap(size());
    pixmap.fill(QColor(24, 25, 29));
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    if (m_currentRoot && !m_tiles.empty()) {
        renderTreemap(&painter, rect());
    }
    return pixmap;
}

QRectF TreemapWidget::findTileRectForNode(DiskNode* node) const {
    if (!node) return QRectF();
    for (const auto& tile : m_tiles) {
        if (tile.node == node) {
            return tile.rect;
        }
    }
    // Ancestor fallback
    for (const auto& tile : m_tiles) {
        DiskNode* curr = node;
        while (curr) {
            if (tile.node == curr) {
                return tile.rect;
            }
            curr = curr->parent();
        }
    }
    return QRectF();
}

void TreemapWidget::renderTreemap(QPainter* painter, const QRectF& bounds) {
    Q_UNUSED(bounds);
    QFont labelFont = font();
    labelFont.setPointSize(8);
    labelFont.setWeight(QFont::Medium);
    painter->setFont(labelFont);

    const auto& tokens = ThemeManager::instance().tokens();

    for (const auto& tile : m_tiles) {
        DiskNode* node = tile.node;
        QRectF r = tile.rect;

        // Base color
        QColor baseColor;
        if (m_colorMode == ColorMode::FileAge) {
            baseColor = DiskNode::getColorForAge(node->lastModifiedTime());
        } else {
            if (node->isDirectory()) {
                baseColor = tokens.surfaceHover.darker(110);
            } else {
                baseColor = DiskNode::getColorForExtension(node->extension());
            }
        }

        bool isHovered = (node == m_hoveredNode);
        bool isSelected = (node == m_selectedNode);

        // Cushion gradient effect with specular lighting
        QLinearGradient grad(r.topLeft(), r.bottomRight());
        if (isHovered) {
            grad.setColorAt(0.0, baseColor.lighter(138));
            grad.setColorAt(0.5, baseColor.lighter(118));
            grad.setColorAt(1.0, baseColor);
        } else if (isSelected) {
            grad.setColorAt(0.0, baseColor.lighter(130));
            grad.setColorAt(0.5, baseColor.lighter(110));
            grad.setColorAt(1.0, baseColor.darker(110));
        } else {
            grad.setColorAt(0.0, baseColor.lighter(118));
            grad.setColorAt(0.6, baseColor);
            grad.setColorAt(1.0, baseColor.darker(130));
        }

        // Draw tile with rounded corners and cushioning
        if (r.width() >= 6.0 && r.height() >= 6.0) {
            qreal cornerRadius = qMin(3.0, qMin(r.width(), r.height()) / 4.0);
            QPainterPath path;
            QRectF innerRect = r.adjusted(0.5, 0.5, -0.5, -0.5);
            path.addRoundedRect(innerRect, cornerRadius, cornerRadius);

            painter->fillPath(path, grad);

            if (isSelected) {
                // Radiant selection glow
                painter->setPen(QPen(QColor(255, 215, 0, 240), 2.0));
                painter->drawPath(path);
            } else if (isHovered) {
                painter->setPen(QPen(QColor(255, 255, 255, 230), 1.5));
                painter->drawPath(path);
            } else {
                painter->setPen(QPen(tokens.surfaceDeep, 1.0));
                painter->drawPath(path);
            }
        } else {
            painter->fillRect(r, baseColor);
        }

        // Text label if space permits
        if (r.width() >= 48.0 && r.height() >= 24.0) {
            QRectF textRect = r.adjusted(4, 3, -4, -2);
            QString displayText = node->name();
            QFontMetrics fm(labelFont);
            QString elidedName = fm.elidedText(displayText, Qt::ElideMiddle, static_cast<int>(textRect.width()));

            // Drop shadow for crisp text over any gradient color
            painter->setPen(QColor(0, 0, 0, 160));
            painter->drawText(textRect.adjusted(1, 1, 1, 1), Qt::AlignTop | Qt::AlignLeft, elidedName);

            painter->setPen(QColor(245, 248, 252));
            painter->drawText(textRect, Qt::AlignTop | Qt::AlignLeft, elidedName);

            if (r.height() >= 38.0) {
                QRectF sizeRect = textRect.adjusted(0, fm.height() + 1, 0, 0);
                QString sizeStr = DiskNode::formatSize(node->size());
                painter->setPen(QColor(0, 0, 0, 140));
                painter->drawText(sizeRect.adjusted(1, 1, 1, 1), Qt::AlignTop | Qt::AlignLeft, sizeStr);
                painter->setPen(QColor(210, 220, 235, 220));
                painter->drawText(sizeRect, Qt::AlignTop | Qt::AlignLeft, sizeStr);
            }
        }
    }
}

void TreemapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Background from theme tokens
    const auto& tokens = ThemeManager::instance().tokens();
    painter.fillRect(rect(), tokens.surfaceDeep);

    // Handle Active Zoom Animation
    if (m_isAnimating && !m_prevPixmap.isNull() && !m_nextPixmap.isNull()) {
        qreal t = m_animProgress;
        QRectF fullRect = rect();
        QRectF target = m_targetTileRect;
        if (target.isEmpty() || target.width() <= 0 || target.height() <= 0) {
            target = QRectF(width() * 0.25, height() * 0.25, width() * 0.5, height() * 0.5);
        }

        if (m_isZoomIn) {
            // Target tile expands from target to fullRect
            QRectF currentTileRect(
                target.left() * (1.0 - t) + fullRect.left() * t,
                target.top() * (1.0 - t) + fullRect.top() * t,
                target.width() * (1.0 - t) + fullRect.width() * t,
                target.height() * (1.0 - t) + fullRect.height() * t
            );

            // Draw outgoing view expanding and fading out
            qreal scale = currentTileRect.width() / target.width();
            qreal offsetX = currentTileRect.left() - target.left() * scale;
            qreal offsetY = currentTileRect.top() - target.top() * scale;

            painter.save();
            painter.setOpacity(qMax(0.0, 1.0 - t));
            painter.translate(offsetX, offsetY);
            painter.scale(scale, scale);
            painter.drawPixmap(0, 0, m_prevPixmap);
            painter.restore();

            // Draw incoming view expanding from target to fullRect and fading in
            painter.save();
            painter.setOpacity(qMin(1.0, t));
            painter.drawPixmap(currentTileRect.toRect(), m_nextPixmap);
            painter.restore();
        } else {
            // Target tile shrinks from fullRect down to target
            QRectF currentChildRect(
                fullRect.left() * (1.0 - t) + target.left() * t,
                fullRect.top() * (1.0 - t) + target.top() * t,
                fullRect.width() * (1.0 - t) + target.width() * t,
                fullRect.height() * (1.0 - t) + target.height() * t
            );

            // Draw incoming parent view fading in
            painter.save();
            painter.setOpacity(qMin(1.0, t));
            painter.drawPixmap(fullRect.toRect(), m_nextPixmap);
            painter.restore();

            // Draw outgoing child view shrinking into target and fading out
            painter.save();
            painter.setOpacity(qMax(0.0, 1.0 - t));
            painter.drawPixmap(currentChildRect.toRect(), m_prevPixmap);
            painter.restore();
        }
        return;
    }

    if (!m_currentRoot || m_tiles.empty()) {
        painter.setPen(QColor(130, 140, 155));
        painter.drawText(rect(), Qt::AlignCenter, 
                         m_currentRoot ? QStringLiteral("Folder is empty or contains 0-byte files")
                                       : QStringLiteral("No scan data available. Start a scan to view treemap."));
        return;
    }

    renderTreemap(&painter, rect());
}

void TreemapWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isAnimating) {
        QToolTip::hideText();
        return;
    }

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
                "<tr><td><b>Modified:</b></td><td style='padding-left:8px;'>%5 (%6)</td></tr>"
                "<tr><td><b>Path:</b></td><td style='padding-left:8px;'>%7</td></tr>"
                "</table>"
            ).arg(m_hoveredNode->name().toHtmlEscaped(),
                 DiskNode::formatSize(m_hoveredNode->size()),
                 QString::number(percent, 'f', 1),
                 m_hoveredNode->isDirectory() ? QStringLiteral("Folder") : m_hoveredNode->extension().toUpper(),
                 m_hoveredNode->lastModifiedTime() > 0 
                     ? QDateTime::fromSecsSinceEpoch(m_hoveredNode->lastModifiedTime()).toString(QStringLiteral("yyyy-MM-dd hh:mm"))
                     : QStringLiteral("Unknown"),
                 DiskNode::formatAge(m_hoveredNode->lastModifiedTime()),
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
    if (m_isAnimating) return;

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
    if (m_isAnimating) return;

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
    if (m_isAnimating) return;

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
