#pragma once

#ifndef Diagram_visualization_h
#define Diagram_visualization_h

// My includes
#include "Point2D.h"
#include "NewDiagram.h"

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, std::vector<std::pair<Point2D, double>>& points);

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, const std::vector<std::pair<Point2D, double>>& points, bool saved, bool minKnapsack, std::string fileName);

#endif
