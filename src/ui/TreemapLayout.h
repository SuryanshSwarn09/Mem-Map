#pragma once

#include <QRectF>
#include <vector>
#include "DiskNode.h"

struct TreemapTile {
    DiskNode* node{nullptr};
    QRectF rect;
    int depth{0};
};

class TreemapLayout {
public:
    static std::vector<TreemapTile> compute(DiskNode* rootNode, const QRectF& bounds, int maxDepth = 2);

private:
    static void layoutChildren(DiskNode* parent, const QRectF& bounds, int depth, int maxDepth, std::vector<TreemapTile>& outTiles);
    static void squarify(const std::vector<DiskNode*>& nodes, const std::vector<double>& areas, QRectF bounds, int depth, int maxDepth, std::vector<TreemapTile>& outTiles);
    static double worstAspect(const std::vector<double>& row, double sideLength);
};
