#include "TreemapLayout.h"
#include <algorithm>
#include <numeric>
#include <cmath>

std::vector<TreemapTile> TreemapLayout::compute(DiskNode* rootNode, const QRectF& bounds, int maxDepth) {
    std::vector<TreemapTile> tiles;
    if (!rootNode || bounds.width() <= 4.0 || bounds.height() <= 4.0 || rootNode->size() == 0) {
        return tiles;
    }

    layoutChildren(rootNode, bounds, 0, maxDepth, tiles);
    return tiles;
}

double TreemapLayout::worstAspect(const std::vector<double>& row, double sideLength) {
    if (row.empty() || sideLength <= 0.0001) return 1e9;
    double sum = 0.0;
    double minVal = row[0];
    double maxVal = row[0];

    for (double a : row) {
        sum += a;
        if (a < minVal) minVal = a;
        if (a > maxVal) maxVal = a;
    }

    if (sum <= 0.0001 || minVal <= 0.0001) return 1e9;

    double s2 = sum * sum;
    double w2 = sideLength * sideLength;

    double aspect1 = (w2 * maxVal) / s2;
    double aspect2 = s2 / (w2 * minVal);
    return std::max(aspect1, aspect2);
}

void TreemapLayout::layoutChildren(DiskNode* parent, const QRectF& bounds, int depth, int maxDepth, std::vector<TreemapTile>& outTiles) {
    if (!parent || bounds.width() < 3.0 || bounds.height() < 3.0) return;

    std::vector<DiskNode*> validChildren;
    double totalChildSize = 0;

    for (const auto& child : parent->children()) {
        if (child->size() > 0) {
            validChildren.push_back(child.get());
            totalChildSize += static_cast<double>(child->size());
        }
    }

    if (validChildren.empty() || totalChildSize <= 0) {
        return;
    }

    // Convert byte sizes into normalized area values covering the bounds area
    double totalArea = bounds.width() * bounds.height();
    std::vector<double> areas;
    areas.reserve(validChildren.size());

    for (DiskNode* node : validChildren) {
        areas.push_back((static_cast<double>(node->size()) / totalChildSize) * totalArea);
    }

    squarify(validChildren, areas, bounds, depth, maxDepth, outTiles);
}

void TreemapLayout::squarify(const std::vector<DiskNode*>& nodes, const std::vector<double>& areas,
                            QRectF bounds, int depth, int maxDepth, std::vector<TreemapTile>& outTiles) {
    size_t startIdx = 0;
    size_t totalNodes = nodes.size();

    while (startIdx < totalNodes && bounds.width() >= 3.0 && bounds.height() >= 3.0) {
        bool horizontal = (bounds.width() >= bounds.height());
        double sideLength = horizontal ? bounds.height() : bounds.width();

        std::vector<double> currentRow;
        std::vector<DiskNode*> currentNodes;

        currentRow.push_back(areas[startIdx]);
        currentNodes.push_back(nodes[startIdx]);
        double currentWorst = worstAspect(currentRow, sideLength);

        size_t nextIdx = startIdx + 1;
        while (nextIdx < totalNodes) {
            std::vector<double> testRow = currentRow;
            testRow.push_back(areas[nextIdx]);
            double testWorst = worstAspect(testRow, sideLength);

            if (testWorst <= currentWorst) {
                currentRow.push_back(areas[nextIdx]);
                currentNodes.push_back(nodes[nextIdx]);
                currentWorst = testWorst;
                nextIdx++;
            } else {
                break;
            }
        }

        // Layout current row in the strip along the side
        double rowSum = std::accumulate(currentRow.begin(), currentRow.end(), 0.0);
        double stripThickness = (sideLength > 0.0001) ? (rowSum / sideLength) : 0.0;

        double offset = 0.0;
        for (size_t i = 0; i < currentNodes.size(); ++i) {
            DiskNode* node = currentNodes[i];
            double itemArea = currentRow[i];
            double itemLength = (stripThickness > 0.0001) ? (itemArea / stripThickness) : 0.0;

            QRectF tileRect;
            if (horizontal) {
                tileRect = QRectF(bounds.x(), bounds.y() + offset, stripThickness, itemLength);
                offset += itemLength;
            } else {
                tileRect = QRectF(bounds.x() + offset, bounds.y(), itemLength, stripThickness);
                offset += itemLength;
            }

            // Only add if visible
            if (tileRect.width() >= 1.5 && tileRect.height() >= 1.5) {
                // If it's a directory and we haven't hit maxDepth, we can recurse into it
                if (node->isDirectory() && depth < maxDepth && tileRect.width() >= 20.0 && tileRect.height() >= 20.0) {
                    // Slight padding for container directory visual separation
                    QRectF innerBounds = tileRect.adjusted(1, 1, -1, -1);
                    layoutChildren(node, innerBounds, depth + 1, maxDepth, outTiles);
                } else {
                    outTiles.push_back(TreemapTile{node, tileRect, depth});
                }
            }
        }

        // Slice off the strip from bounds
        if (horizontal) {
            bounds.setX(bounds.x() + stripThickness);
        } else {
            bounds.setY(bounds.y() + stripThickness);
        }

        startIdx = nextIdx;
    }
}
