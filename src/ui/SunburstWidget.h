#pragma once

#include <QWidget>
#include <vector>
#include <memory>
#include <QPainterPath>
#include "DiskNode.h"

struct SunburstSlice {
    DiskNode* node{nullptr};
    double innerRadius{0};
    double outerRadius{0};
    double startAngle{0};
    double spanAngle{0};
    int depth{0};
    QPainterPath path;
};

class SunburstWidget : public QWidget {
    Q_OBJECT

public:
    explicit SunburstWidget(QWidget* parent = nullptr);
    ~SunburstWidget() override = default;

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
    void computeLayout();
    void buildSlices(DiskNode* parent, double startAngle, double spanAngle,
                     double innerR, double ringWidth, int depth, int maxDepth,
                     const QPointF& center);
    SunburstSlice* sliceAt(const QPointF& pos);
    bool isCenterHub(const QPointF& pos) const;

    DiskNode* m_dataRoot{nullptr};
    DiskNode* m_currentRoot{nullptr};
    DiskNode* m_selectedNode{nullptr};
    DiskNode* m_hoveredNode{nullptr};
    bool m_hoveredCenter{false};

    std::vector<SunburstSlice> m_slices;
    QPointF m_center;
    double m_hubRadius{60.0};
    int m_maxDepth{4};
};
