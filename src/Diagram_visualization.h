#pragma once

#ifndef Diagram_visualization_h
#define Diagram_visualization_h

// My includes
#include "Point2D.h"
#include "NewDiagram.h"

struct BoundaryElement {
    bool isRay;

    // if segment
    Point2D a, b;

    // if ray
    Point2D origin;

    Point2D rayDirection;     // for drawing
    Point2D normalDirection;  // for inside test
};

struct RegionData {
    std::vector<BoundaryElement> boundary;  // ORDERED (clockwise)
    std::vector<size_t> siteIndices;
};

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, std::vector<std::pair<Point2D, double>>& points);

void visualize_diagram(std::vector<RegionData>& regions, std::vector<Point2D>& points);

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, const std::vector<std::pair<Point2D, double>>& points, bool saved, bool minKnapsack, std::string fileName);

#endif
