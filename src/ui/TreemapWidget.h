#pragma once

#include <QWidget>
#include <vector>
#include <memory>
#include "TreemapLayout.h"
#include "DiskNode.h"

class TreemapWidget : public QWidget {
    Q_OBJECT

public:
    explicit TreemapWidget(QWidget* parent = nullptr);
    ~TreemapWidget() override = default;

    void setRootNode(DiskNode* rootNode);
    void zoomIn(DiskNode* node);
    void zoomOut();
    void selectNode(DiskNode* node);
    DiskNode* currentRoot() const { return m_currentRoot; }
    DiskNode* selectedNode() const { return m_selectedNode; }

signals:
    void nodeSelected(DiskNode* node);
    void nodeDoubleClicked(DiskNode* node);
    void currentRootChanged(DiskNode* newRoot);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void relayout();
    TreemapTile* tileAt(const QPointF& pos);

    DiskNode* m_dataRoot{nullptr};     // Root of the whole scan
    DiskNode* m_currentRoot{nullptr};  // Currently zoomed directory
    DiskNode* m_selectedNode{nullptr};
    DiskNode* m_hoveredNode{nullptr};

    std::vector<TreemapTile> m_tiles;
    int m_maxDepth{2};
};
