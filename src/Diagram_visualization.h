#pragma once

#ifndef Diagram_visualization_h
#define Diagram_visualization_h

// My includes
#include <optional>
#include "Point2D.h"
#include "NewDiagram.h"

struct BoundaryElement {
    bool isRay;

    // if it's a ray only one of the two points is significant, the other is just a placeholder
    bool hasA;
    bool hasB;

    Point2D a, b; // a is the tail and b is the head

    Point2D direction;
    Point2D geomDirection;
};

struct RegionData {
    std::vector<BoundaryElement> boundary;  // ORDERED (clockwise)
    std::vector<size_t> siteIndices;
};

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, std::vector<std::pair<Point2D, double>>& points);

void visualize_diagram(std::vector<RegionData>& regions, std::vector<Point2D>& points);

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, const std::vector<std::pair<Point2D, double>>& points, bool saved, int minKnapsack, std::string fileName);

#endif
