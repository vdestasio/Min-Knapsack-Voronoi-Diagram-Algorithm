#include <iostream>
#include <memory>
#include <fstream> 
#include <stdio.h>
#include <MyGAL/FortuneAlgorithm.h>
#include "MinKnapsack.h"
#include <filesystem>
#include "Diagram_visualization.h"
// #include <Eigen/Dense>

namespace vor = Voronoi;


void loadRegions(
    const std::string& filename,
    std::vector<RegionData>& regions)
{
    std::ifstream in(filename, std::ios::binary);

    if (!in) {
        std::cerr << "Cannot open file: " << filename << "\n";
        exit(1);
    }

    auto readSize = [&](size_t& v) {
        in.read(reinterpret_cast<char*>(&v), sizeof(size_t));
        if (!in) {
            std::cerr << "Error reading size_t\n";
            exit(1);
        }
    };

    auto readBool = [&](bool& v) {
        in.read(reinterpret_cast<char*>(&v), sizeof(bool));
        if (!in) {
            std::cerr << "Error reading bool\n";
            exit(1);
        }
    };

    auto readDouble = [&](double& v) {
        in.read(reinterpret_cast<char*>(&v), sizeof(double));
        if (!in) {
            std::cerr << "Error reading double\n";
            exit(1);
        }
    };

    auto readPoint = [&](Point2D& p) {
        readDouble(p.x);
        readDouble(p.y);
    };

    size_t nRegions;
    readSize(nRegions);

    regions.clear();
    regions.resize(nRegions);

    for (size_t i = 0; i < nRegions; ++i) {

        // ===== boundary =====
        size_t nB;
        readSize(nB);
        regions[i].boundary.resize(nB);

        for (size_t j = 0; j < nB; ++j) {
            bool isRay;
            readBool(isRay);

            regions[i].boundary[j].isRay = isRay;

            if (!isRay) {
                // segment
                readPoint(regions[i].boundary[j].a);
                readPoint(regions[i].boundary[j].b);
            } else {
                // ray
                readPoint(regions[i].boundary[j].origin);
                readPoint(regions[i].boundary[j].direction);
            }
        }

        // ===== site indices =====
        size_t nS;
        readSize(nS);
        regions[i].siteIndices.resize(nS);

        for (size_t j = 0; j < nS; ++j) {
            readSize(regions[i].siteIndices[j]);
        }
    }

    in.close();
}

double valueFunction(const Point2D& x, const std::vector<size_t>& siteIndices, const std::vector<Point2D>& sitePoints, const std::vector<double>& siteCaps, double capacity) {
    double value = 0.0;
    for (size_t idx : siteIndices) {
        double d = hypot(x.x - sitePoints[idx].x, x.y - sitePoints[idx].y);
        if (capacity < siteCaps[idx]) {
            value += (capacity) * d;
        }else{
            value += siteCaps[idx] * d;
        }
        capacity -= siteCaps[idx];
    }

    return value;
}

Point2D weiszfeld(const std::vector<Point2D>& pts, Point2D x0) {
    Point2D x = x0;

    for (int iter = 0; iter < 100; ++iter) {
        double num_x = 0, num_y = 0, denom = 0;

        for (const auto& p : pts) {
            double d = hypot(x.x - p.x, x.y - p.y);
            if (d < 1e-10) {    
                return p;
            }

            num_x += p.x / d;
            num_y += p.y / d;
            denom += 1.0 / d;
        }
        // std::cout << "Weiszfeld iteration " << iter << ": x = " << x << "\n";
        // std::cout << "  num_x = " << num_x << ", num_y = " << num_y << ", denom = " << denom << "\n";
        if (denom < 1e-12) break;
        
        Point2D new_x{num_x / denom, num_y / denom};

        if (hypot(new_x.x - x.x, new_x.y - x.y) < 1e-8)
            break;

        x = new_x;
    }

    return x;
}


bool isInsideRegion(const RegionData& r, const Point2D& x)
{
    const double eps = 1e-12;

    for (const auto& edge : r.boundary) {

        if (!edge.isRay) {
            Point2D dir = edge.b - edge.a;
            if (crossProduct(dir, x - edge.a) > eps)
                return false;
        }
        else {
            if (crossProduct(edge.direction, x - edge.origin) > eps)
                return false;
        }
    }

    return true;
}

// using Vector = Eigen::VectorXd;
// using Matrix = Eigen::MatrixXd;

// Vector newton(
//     std::function<Vector(const Vector&)> grad,
//     std::function<Matrix(const Vector&)> hess,
//     Vector x0,
//     int maxIter = 50
// ) {
//     Vector x = x0;

//     for (int i = 0; i < maxIter; ++i) {
//         Vector g = grad(x);
//         Matrix H = hess(x);

//         Vector dx = H.ldlt().solve(-g);

//         x += dx;

//         if (dx.norm() < 1e-8) break;
//     }

//     return x;
// }

int main(int argc, char* argv[]) {
    // Default value
    std::string fileName = "Data/equilatero_norm.txt";
    bool visualize = true;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--file" && i + 1 < argc) {
            fileName = argv[++i];  // Take the next argument as filename
        }        else if (arg == "--visualize") {
            if (i + 1 < argc) {                   // Make sure there is a next argument
                std::string val = argv[i + 1];    // Read it
                if (val == "1") visualize = true;
                else if (val == "0") visualize = false;
                else {
                    std::cerr << "Invalid value for --visualize: " << val << "\n";
                    return 1;
                }
                i++; // Skip the value for next iteration
            } else {
                std::cerr << "--visualize requires 0 or 1\n";
                return 1;
            }
        }else {
            std::cerr << "Unknown argument: " << arg << std::endl;
        }
    }

    std::cout << "File: " << fileName << "\n";
    std::string fullPath = "Data/Saved_diagrams/" +  std::filesystem::path(fileName).stem().string() + ".bin";
    std::ifstream in(fileName);
    std::vector<Point2D> sitePoints;
    std::vector<double> siteCaps;
    double x, y, weight;
    int points_n = 0, i = 0;
    double capacity = 0.0;
    if (in) {
        in >> capacity;
        in >> points_n;
        for (int p_i = 0; p_i < points_n; ++p_i) {
            in >> x >> y >> weight;
            if (p_i == i) {
                sitePoints.push_back(Point2D(x, y));
                siteCaps.push_back(weight);
                i += 1;
            }
        }
    }
    std::cout << "Read " << sitePoints.size() << " sites with capacity: " << capacity << "\n";


    std::vector<Point2D> local_optima;
    std::vector<double> local_optima_cost;
    std::vector<std::vector<Point2D>> local_optima_sites;

    std::vector<RegionData> regions;

    loadRegions(fullPath, regions);
    std::cout << "Loaded " << regions.size() << " regions\n";

    // For each region find the corresponding solution with weiszfeld and check if it's inside the region,
    // if it is, compute the cost of that solution and add it to the local optima
    // if the local optima is the smallest one, update the global optimum
    for (auto& r : regions) {
        std::vector<Point2D> tmp_sites;
        for (const auto& s : r.siteIndices){
            tmp_sites.push_back(sitePoints[s]);
        }

        Point2D x0 = Point2D(0,0);
        for (auto idx : r.siteIndices) {
            x0 += sitePoints[idx];
        }
        x0 /= (double)r.siteIndices.size();

        Point2D sol;
        std::cout << "Region with sites: ";
        for (const auto& s : r.siteIndices){
            std::cout << sitePoints[s] << " ";
        }
        std::cout << "\n";
        if (tmp_sites.size() == 1) {
            sol = tmp_sites[0];
            std::cout << "Single vertex region, solution is the site: " << sol << "\n";
        }else if (tmp_sites.size() == 2) {
            Point2D a = tmp_sites[0];
            Point2D b = tmp_sites[1];
            sol = Point2D((a.x + b.x) * 0.5, (a.y + b.y) * 0.5);
            std::cout << "Two vertices region, solution is the midpoint: " << sol << "\n";
        }else{
            sol = weiszfeld(tmp_sites, x0); //TODO should this be weighted somehow??
            std::cout << "Weiszfeld solution: " << sol << "\n";
        }
        
        if (isInsideRegion(r, sol)){
            local_optima.push_back(sol);
            local_optima_cost.push_back(valueFunction(sol,r.siteIndices, sitePoints, siteCaps, capacity)); 
            local_optima_sites.push_back(tmp_sites);
            std::cout << "Local optimum for region with sites: ";
            for (const auto& s : r.siteIndices){
                std::cout << sitePoints[s] << " ";
            }
            std::cout << "is: " << sol << " with cost: " << local_optima_cost.back() << "\n";
        }else{
            std::cout << "Solution is outside the region, skipping...\n";
        }
    }
    if (local_optima.size() == 0) {
        std::cout << "ERROR: No local optima found inside any region.\n";
        return 0;
    }

    Point2D global_optimum = local_optima[0];
    double global_optimum_cost = local_optima_cost[0];
    std::vector<Point2D> global_optimum_sites = local_optima_sites[0];
    for (size_t i = 1; i < local_optima.size(); ++i) {
        if (local_optima_cost[i] < global_optimum_cost) {
            global_optimum = local_optima[i];
            global_optimum_cost = local_optima_cost[i];
            global_optimum_sites = local_optima_sites[i];
        }
    }
    std::cout << "Global optimum point: " << global_optimum << " with cost: " << global_optimum_cost << " and with sites: ";
    for (const auto& s : global_optimum_sites){
        std::cout << s << " ";
    }
    std::cout << "\n";
    
    if (visualize) {
        std::cout << "Visualizing diagram with local optima...\n";
        std::vector<Point2D> sites_with_minimum = sitePoints;
        sites_with_minimum.push_back(global_optimum); // add global optimum as a site for visualization purposes
        visualize_diagram(regions, sites_with_minimum);
    }
        
    return 0;
}
 
