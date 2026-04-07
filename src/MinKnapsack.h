#pragma once

#ifndef MinKnapsack_h
#define MinKnapsack_h

// STL
#include <vector>
// My includes
#include "Point2D.h"
#include "NewDiagram.h"

std::list<Voronoi::NewDiagram::FacePtr> build_minKnapsack(Voronoi::NewDiagram& diagram, std::vector<std::pair<Point2D, double>>& points, double total, bool save_image, const std::string& fileName);

#endif
