#include <iostream>
#include <memory>
#include <fstream> 
#include <stdio.h>
#include <MyGAL/FortuneAlgorithm.h>
#include "MinKnapsack.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
// #include <Eigen/Dense>

namespace vor = Voronoi;

// Global variable to keep track of the current number of region, TODO: implement better solution
long progressiveID = 0;

double readCapacity(const std::string& fileName, int step = 1) {
    std::ifstream in(fileName);
    double capacity = 0.0;
    if (in) {
        in >> capacity;
    }
    return capacity;
}

// Function to read the points and their weights from a given file
std::vector<std::pair<Point2D, double>> readPoints(const std::string& fileName, int step = 1) {

    std::vector<std::pair<Point2D, double>> points;
    std::ifstream in(fileName);

    double x, y, weight;
    int points_n = 0, i = 0;
    double capacity = 0.0;

    if (in) {
        in >> capacity;
        in >> points_n;
        for (int p_i = 0; p_i < points_n; ++p_i) {
            in >> x >> y >> weight;
            if (p_i == i) {
                points.push_back(std::make_pair(Point2D(x, y), weight));
                i += step;
            }
        }
    }
    return points;
}

// Function to draw a grid
void drawGrid(sf::RenderWindow& window, int gridSpacing) {
    sf::VertexArray gridLines(sf::Lines);

    sf::Vector2u windowSize = window.getSize();

    // Vertical lines
    for (int x = 0; x < windowSize.x; x += gridSpacing) {
        gridLines.append(sf::Vertex(sf::Vector2f(x, 0), sf::Color(200, 200, 200)));  // Light gray color
        gridLines.append(sf::Vertex(sf::Vector2f(x, windowSize.y), sf::Color(200, 200, 200)));
    }

    // Horizontal lines
    for (int y = 0; y < windowSize.y; y += gridSpacing) {
        gridLines.append(sf::Vertex(sf::Vector2f(0, y), sf::Color(200, 200, 200)));
        gridLines.append(sf::Vertex(sf::Vector2f(windowSize.x, y), sf::Color(200, 200, 200)));
    }

    window.draw(gridLines);
}

// Function to initialize edge points for SFML
void initEdgePointsVis(Voronoi::NewDiagram::HalfEdgePtr& h, sf::VertexArray& line,
    const std::vector<std::pair<Point2D,double>>& points) 
{
    if (!h) return; // sanity check

    // Safe pointers
    auto head = h->head;
    auto twin = h->twin;
    auto twinHead = (twin) ? twin->head : nullptr;

    // Case 1: both vertices defined
    if (head && !head->infinite && twinHead && !twinHead->infinite) {
        line[0].position = sf::Vector2f(head->point.x, head->point.y);
        line[1].position = sf::Vector2f(twinHead->point.x, twinHead->point.y);
    }
    // Case 2: only head defined
    else if (head && !head->infinite && twin && twin->label) {
        line[0].position = sf::Vector2f(head->point.x, head->point.y);
        Point2D norm = (points[twin->label->index].first - points[h->label->index].first)
                       .normalized().getRotated90CCW();
        line[1].position = sf::Vector2f(line[0].position.x + norm.x * 1000,
                                        line[0].position.y + norm.y * 1000);
    }
    // Case 3: only twin's head defined
    else if (twinHead && !twinHead->infinite && h->label && twin->label) {
        line[0].position = sf::Vector2f(twinHead->point.x, twinHead->point.y);
        Point2D norm = (points[h->label->index].first - points[twin->label->index].first)
                       .normalized().getRotated90CCW();
        line[1].position = sf::Vector2f(line[0].position.x + norm.x * 1000,
                                        line[0].position.y + norm.y * 1000);
    }
    // Case 4: no vertices defined
    else if (h->label && twin && twin->label) {
        Point2D p1 = points[twin->label->index].first;
        Point2D p2 = points[h->label->index].first;
        Point2D norm = (p1 - p2).normalized().getRotated90CCW();
        Point2D c = 0.5 * (p1 + p2);
        line[0].position = sf::Vector2f(c.x + norm.x * 1000, c.y + norm.y * 1000);
        line[1].position = sf::Vector2f(c.x - norm.x * 1000, c.y - norm.y * 1000);
    } 
    else {
        // Fallback: set both points to origin to avoid invalid memory access
        line[0].position = sf::Vector2f(0.f, 0.f);
        line[1].position = sf::Vector2f(0.f, 0.f);
    }

    // Set the color of the line
    line[0].color = sf::Color::Green;
    line[1].color = sf::Color::Green;
}


// ChatGPT function that deletes all pointers references and assures that there are no leaks (used at the very end of the program)
void cleanupDiagram(Voronoi::NewDiagram& diagram) {
    // --- 1. Delete all halfedges safely ---
    std::unordered_set<Voronoi::NewDiagram::HalfEdgePtr> deletedEdges;

    for (auto& face : diagram.getFaces()) {
        Voronoi::NewDiagram::HalfEdgePtr start = face->firstEdge;
        if (!start) continue;

        Voronoi::NewDiagram::HalfEdgePtr edge = start;
        do {
            if (deletedEdges.find(edge) == deletedEdges.end()) {
                deletedEdges.insert(edge);
                auto nextEdge = edge->next; // store next before deletion
                diagram.deleteHalfEdge(edge);
                edge = nextEdge;
            } else {
                break; // edge already deleted (likely twin)
            }
        } while (edge != start && edge != nullptr);
    }

    // --- 2. Delete all vertices ---
    // Assuming you kept a vector or map of vertices in the diagram
    // If not, you need to store them when you create them
    for (auto& face : diagram.getFaces()) {
        auto edge = face->firstEdge;
        if (!edge) continue;
        Voronoi::NewDiagram::VertexPtr startVertex = edge->head;
        Voronoi::NewDiagram::VertexPtr vertex = startVertex;
        std::unordered_set<Voronoi::NewDiagram::VertexPtr> deletedVertices;

        do {
            if (deletedVertices.find(vertex) == deletedVertices.end()) {
                deletedVertices.insert(vertex);
                vertex.reset(); // shared_ptr drops ref count
                vertex = edge->next ? edge->next->head : nullptr;
            } else {
                break;
            }
        } while (vertex != startVertex && vertex != nullptr);
    }

    // --- 3. Delete all faces ---
    for (auto& face : diagram.getFaces()) {
        face.reset();
    }

    // Optional: clear the faces list if you want to completely reset diagram
    diagram.getFaces().clear();
}



// Function to modify the old Voronoi diagram structure to a new one
void modifyStructure(const mygal::Diagram<double>& diagram,
    Voronoi::NewDiagram& newDiagram) {

    // Create a map from old vertices to new vertices
    std::unordered_map<mygal::Diagram<double>::Vertex*, Voronoi::NewDiagram::VertexPtr> mapVertices;
    // Create a map from old halfedges to new halfedges
    std::unordered_map<mygal::Diagram<double>::HalfEdge*, Voronoi::NewDiagram::HalfEdgePtr> mapHalfedges;
    //Create a map from old regions to new regions
    std::unordered_map<mygal::Diagram<double>::Face*, Voronoi::NewDiagram::FacePtr> mapFaces;

    //Connects halfedges & vertices & faces
    for (auto& e : diagram.getHalfEdges()) {
        // I refer to the halfedge based on the pointer of the twin
        if (mapHalfedges.find(e.twin->twin) == mapHalfedges.end()) {
            //?
            mapHalfedges[e.twin->twin] = newDiagram.createHalfEdge(newDiagram.getSite(e.incidentFace->site->index));
        }
        

        if (mapHalfedges.find(e.twin) == mapHalfedges.end()) {
            //?
            mapHalfedges[e.twin] = newDiagram.createHalfEdge(newDiagram.getSite(e.twin->incidentFace->site->index));
        }

        auto& e_new = mapHalfedges.at(e.twin->twin);
        e_new->twin = mapHalfedges.at(e.twin); 
        e_new->twin->twin = mapHalfedges.at(e.twin->twin);

        if (e_new->twin->twin != e_new) {
            std::cerr << "Twin consistency issue!" << std::endl;
        }

        if (mapFaces.find(e.incidentFace) == mapFaces.end()) {
            auto f_incident = newDiagram.createFace(progressiveID);
            f_incident->flag = true;
            f_incident->sites = { newDiagram.getSite(e.incidentFace->site->index) };
            f_incident->weight = newDiagram.getWeight(e.incidentFace->site->index);
            if (f_incident->weight <= newDiagram.getTotal()) {
                f_incident->pivot = nullptr;
            }
            else {
                f_incident->pivot = newDiagram.getSite(e.incidentFace->site->index);
            }
            mapFaces[e.incidentFace] = f_incident;
            progressiveID++;
        }
        e_new->region = mapFaces.at(e.incidentFace);

        // In the original structure the halfedges are connected in anti-clockwise sense,
        // but we need a clockwise sense so we assign the prev halfedge as next
        // and we assign the origin vertex as head and the destination vertex as tail

        // Check if there is a previous halfedge of the old halfedge
        if (e.prev != nullptr) {
            if (mapHalfedges.find(e.prev) == mapHalfedges.end()) {
                //?
                auto e_prev_new = newDiagram.createHalfEdge(newDiagram.getSite(e.prev->incidentFace->site->index));
                mapHalfedges[e.prev] = e_prev_new;
            }
            e_new->next = mapHalfedges.at(e.prev);
        }

        // Check if there is a origin vertex of the old halfedge
        Voronoi::NewDiagram::VertexPtr v1_new;
        if (e.origin != nullptr) {
            if (mapVertices.find(e.origin)==mapVertices.end()) {
                //?
                auto v_origin = newDiagram.createVertex(Point2D(e.origin->point.x, e.origin->point.y));
                v_origin->infinite = false;
                mapVertices[e.origin] = v_origin;
            }
            v1_new = mapVertices.at(e.origin);
        }
        else if (e_new->twin->tail != nullptr) {
            v1_new = e_new->twin->tail;
        }
        else {
            //v1_new = newDiagram.createVertex(NULL);
            //v1_new->infinite = true;
            v1_new = Voronoi::NewDiagram::Vertex::getNullVertex();
        }
        e_new->head = v1_new;


        // Analogous operations with the destination vertex of the old halfedge
        Voronoi::NewDiagram::VertexPtr v2_new;
        if (e.destination != nullptr) {
            if (mapVertices.find(e.destination) == mapVertices.end()) {
                //?
                auto v_destination = newDiagram.createVertex(Point2D(e.destination->point.x, e.destination->point.y));
                v_destination->infinite = false;
                mapVertices[e.destination] = v_destination;
            }
            v2_new = mapVertices.at(e.destination);
        }
        else if (e_new->twin->head != nullptr) {
            v2_new = e_new->twin->head;
        }
        else {
            //v2_new = newDiagram.createVertex(NULL);
            //v2_new->infinite = true;
            v2_new = Voronoi::NewDiagram::Vertex::getNullVertex();
        }
        e_new->tail = v2_new;




        // Assign the halfedge of the associated region if there aren't any already
        // or if this the first halfedge
        if ((e_new->region->firstEdge == nullptr) || (e_new->tail->infinite == true)) {
            e_new->region->firstEdge = e_new;
        }

    }
    //!!! Per ora la tripletta dei vertici la lascio vuota,
    // se serve la riempio sfruttando i semilati

  

    // Iterate again over all the unbounded faces to connect their last halfedge with their first halfedge,
    // this way the halfedges of each region form a circular list
    for (auto& f : newDiagram.getFaces()) {
        if (f->firstEdge->tail->infinite == true) {
            auto e = f->firstEdge;
            while (e->head->infinite != true) {
                e = e->next;
            }
            e->next = f->firstEdge;
        }
    }

    // Iterate again over all the halfedges to assign the right triplet to all vertices,
    // it's possible only now because we needed to connect all halfedges to each other
    for (auto& f : newDiagram.getFaces()) {
        auto e = f->firstEdge;
        do
        {
            auto& v1_new = e->head;
            // If the vertex to which the new halfedge points has not a triplet yet,
            // we create it based on the old halfedges that had it as their origin
            if (!v1_new->infinite && v1_new->triplet.empty()) {
                std::vector<Voronoi::NewDiagram::SitePtr> temp = std::vector<Voronoi::NewDiagram::SitePtr>(3);
                temp[0] = e->label;
                temp[1] = e->next->twin->label;
                temp[2] = e->next->twin->next->twin->label;
                v1_new->triplet = temp;
            }
            e = e->next;
        } while (e!=f->firstEdge);
    }

}

bool isInsideConvex(const std::vector<Point2D>& poly, const Point2D& p) {
    int n = poly.size();
    if (n < 3) return false;

    for (int i = 0; i < n; ++i) {
        Point2D a = poly[i];
        Point2D b = poly[(i + 1) % n];

        if (crossProduct(b - a, p - a) < 0) {
            return false; // outside
        }
    }
    return true;
}

std::vector<Point2D> extractRegionVertices(Voronoi::NewDiagram::FacePtr face){
    std::vector<Point2D> pts;

    Voronoi::NewDiagram::HalfEdgePtr start = face->firstEdge;
    Voronoi::NewDiagram::HalfEdgePtr e = start;

    do {
        Voronoi::NewDiagram::VertexPtr v = e->tail;

        if (!v->infinite) {
            pts.push_back(v->point);
        }

        e = e->next;
    } while (e != start);

    return pts;
}

Point2D weiszfeld(const std::vector<Point2D>& pts, Point2D x0) {
    Point2D x = x0;

    for (int iter = 0; iter < 100; ++iter) {
        double num_x = 0, num_y = 0, denom = 0;

        for (const auto& p : pts) {
            double d = hypot(x.x - p.x, x.y - p.y);
            if (d < 1e-10) continue;

            num_x += p.x / d;
            num_y += p.y / d;
            denom += 1.0 / d;
        }

        Point2D new_x{num_x / denom, num_y / denom};

        if (hypot(new_x.x - x.x, new_x.y - x.y) < 1e-8)
            break;

        x = new_x;
    }

    return x;
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

    // Valgrind doesn't work with SFML/Graphics!
    // so you need to put this variable to false to be able to run Valgrind analysis correctly
    bool visualize = true;

    bool minKnapsack = true;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--file" && i + 1 < argc) {
            fileName = argv[++i];  // Take the next argument as filename
        } 
        else if (arg == "--visualize") {
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
        }else if (arg=="--minKnapsack"){
            if (i +1 < argc){
                std::string val = argv[i + 1];
                if (val == "0") minKnapsack = false;
                else if (val=="1") minKnapsack = true;
                else {
                    std::cerr << "Invalid value for --minKnapsack: " << val << "\n";
                    return 1;
                }
                i++;
            }
        }else {
            std::cerr << "Unknown argument: " << arg << std::endl;
        }
    }

    std::cout << "File: " << fileName << "\n";
    std::cout << "Visualize: " << (visualize ? "true" : "false") << "\n";


    // Read points
    std::vector<std::pair<Point2D, double>> points_with_weights = readPoints(fileName);
    double capacity = readCapacity(fileName);

    std::vector<mygal::Vector2<double>> points;
    for (size_t i = 0; i < points_with_weights.size(); ++i) {
        points.push_back(mygal::Vector2<double>(points_with_weights[i].first.x, points_with_weights[i].first.y));
    }
 
    // Initialize an instance of Fortune's algorithm
    auto algorithm = mygal::FortuneAlgorithm<double>(points);
    // Construct the first order Voronoi diagram
    algorithm.construct();
    // Get the constructed diagram
    auto diagram = algorithm.getDiagram();
  
    //Create the diagram with the sites and weights
    auto newDiagram = Voronoi::NewDiagram(points_with_weights, capacity);

    // Modify the diagram in order to get the structure I used in the pseudocode
    modifyStructure(diagram, newDiagram);    
    auto& faces = newDiagram.getFaces();

    //Construct the min-knapsack Voronoi diagram
    faces = build_minKnapsack(newDiagram, points_with_weights, capacity);
    
    // std::cout << "Min Knapsack Voronoi Diagram\n";
    auto& fs = faces;
    // for (const auto& face : fs) {
    //     if (face==nullptr) continue;
    //     std::cout << *face <<"\n";
    // }

    for (const auto& face : fs) {
        std::vector<Point2D> vertices = extractRegionVertices(face);
    }
    // TODO instead of recomputing the diagram, save all vertices of each region (but i also need its sites and pivot!)

    std::vector<Point2D> local_optima;
    std::vector<float> local_optima_cost; // TODO understand how to compute it!!
    std::vector<Voronoi::NewDiagram::SitePtr> local_optima_sites;

    for (const auto& face : fs) {
        auto pts = extractRegionVertices(face);

        if (pts.empty()) continue;

        Point2D x0 = pts[0]; // or centroid

        Point2D sol = weiszfeld(pts, x0); //TODO fix!!!
        
        if (isInsideConvex(pts, sol)){
            local_optima.push_back(sol);
        }

        local_optima_cost.push_back(face->weight);
        for (const auto& s : face->sites){
            local_optima_sites.push_back(s);
        }
        if (face->pivot != nullptr) {
            local_optima_sites.push_back(face->pivot);
        }

    }

    Point2D global_optimum; // TODO select the smallest cost!!!
     


    // Visualize the diagram
    if (visualize==true){
        // Create a window
        sf::RenderWindow window(sf::VideoMode(800, 800), "Plot Points",sf::Style::Close);

        // Create a view that maps from [0, 1] in both axes to the window's size
        sf::View view;
        view.setCenter(0.5f, 0.5f);   // Center the view at (0.5, 0.5)
        view.setSize(1.0f,-1.0f);    // Set the size to (1, -1) to invert the y-axis

        // Apply this view to the window
        window.setView(view);

        // Disable vsync
        window.setVerticalSyncEnabled(false);

        // Define the points to plot
        float radius = 0.01f;
        sf::Color pointColor = sf::Color::Red;
        std::vector<sf::CircleShape> shapes;
        for (const auto& point : points_with_weights) {
            sf::CircleShape shape(radius);
            shape.setFillColor(pointColor);
            shape.setOrigin(radius, radius);
            shape.setPosition(point.first.x, point.first.y);
            shapes.push_back(shape);
        }

        
        sf::VertexArray allEdges(sf::Lines);

        for (auto& face : faces) {
            Voronoi::NewDiagram::HalfEdgePtr edge = face->firstEdge;
            do {
                sf::VertexArray line(sf::Lines, 2);
                initEdgePointsVis(edge, line, points_with_weights);
                allEdges.append(line[0]);
                allEdges.append(line[1]);
                // TODO: sometimes the next is nullpointer, understand WHY it's nullpointer!!!
                edge = edge->next;
            } while (edge && edge != face->firstEdge);
        }

        bool saved = true;
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();

                // Handle window resizing
                if (event.type == sf::Event::Resized) {
                    view.setSize(1.0f,-1.0f* static_cast<float>(event.size.height) / event.size.width);
                    window.setView(view);
                }
            }

            // Clear window with white background
            window.clear(sf::Color::White);

            // Draw the grid
            drawGrid(window, 50);

            // Draw all points
            for (const auto& shape : shapes) {
                window.draw(shape);
            }

            // Draw all edges from the stored sf::VertexArray
            window.draw(allEdges);

            // Display the window's current frame
            window.display();

            if (!saved) {
                sf::Texture texture;
                texture.create(window.getSize().x, window.getSize().y);
                texture.update(window);
                std::string suffix = minKnapsack ? "_MKVD" : "_base";   // change if needed
                std::string folder = "Images";
                std::string fullPath = folder + "/" +  std::filesystem::path(fileName).stem().string() + suffix + ".png";
                texture.copyToImage().saveToFile(fullPath);
                saved = true;
            }
        }
    }


    cleanupDiagram(newDiagram);

        
    return 0;
}
 
