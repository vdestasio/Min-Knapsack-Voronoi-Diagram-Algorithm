#include <iostream>
#include <memory>
#include <fstream>
#include <filesystem> 
#include <stdio.h>
#include <MyGAL/FortuneAlgorithm.h>
#include "MinKnapsack.h"
#include <SFML/Graphics.hpp>
#include "Diagram_visualization.h"

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
    }else{
        std::cerr << "Cannot open file: " << fileName << "\n";
        exit(1);
    }
    return points;
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

RegionData extractRegion(const Voronoi::NewDiagram::FacePtr& face) {
    RegionData region;

    auto start = face->firstEdge;
    auto e = start;

    // --- STEP 1: collect raw edges ---
    do {
        auto s1 = e->label;
        auto s2 = e->twin->label;

        auto tail = e->tail;
        auto head = e->head;
        Point2D edgeDir;
        Point2D geomDir;

        geomDir = ((s2->point - s1->point).getRotated90CCW());
        geomDir.normalize();

        // --- CASE 1: finite edge ---
        if (tail && head && !tail->infinite && !head->infinite) {
            edgeDir = head->point - tail->point;
            edgeDir.normalize();

            region.boundary.push_back({
                false, true, true,
                tail->point,
                head->point,
                edgeDir,
                geomDir
            });
        }
        else {

            edgeDir = (s2->point - s1->point).getRotated90CCW();
            edgeDir.normalize();
            
            // --- CASE 2: infinite edge → ray ---
            if (head && !head->infinite) {
                // ray goes TO head → so direction is backwards
                region.boundary.push_back({
                    true, false, true,
                    {},  
                    head->point,        // head (finite endpoint)
                    -edgeDir,
                    geomDir
                });
            }
            else if (tail && !tail->infinite) {
                // ray starts at tail and goes forward
                region.boundary.push_back({
                    true, true, false,
                    tail->point,        // tail (finite endpoint)
                    {},  
                    edgeDir,
                    -geomDir
                });
            }
        }

        e = e->next;

    } while (e && e != start);

    // --- STEP 2: collect sites ---
    for (auto& s : face->sites)
        region.siteIndices.push_back(s->index);

    if (face->pivot){
        region.siteIndices.push_back(face->pivot->index);
        region.hasPivot = true;
    }

    return region;
}

void saveRegions(const std::list<Voronoi::NewDiagram::FacePtr>& faces, const std::string& filename)
{
    std::ofstream out(filename, std::ios::binary);

    if (!out) {
        std::cerr << "Cannot open file for writing: " << filename << "\n";
        exit(1);
    }

    auto writeSize = [&](size_t v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(size_t));
    };

    auto writeBool = [&](bool v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(bool));
    };

    auto writeDouble = [&](double v) {
        out.write(reinterpret_cast<const char*>(&v), sizeof(double));
    };

    auto writePoint = [&](const Point2D& p) {
        writeDouble(p.x);
        writeDouble(p.y);
    };

    // number of regions
    writeSize(faces.size());

    for (const auto& f : faces) {
        auto r = extractRegion(f);

        // ===== boundary =====
        writeSize(r.boundary.size());

        for (const auto& e : r.boundary) {
            writeBool(e.isRay);
            writeBool(e.hasA);
            writeBool(e.hasB);

            if (!e.isRay) {
                // segment
                writePoint(e.a);
                writePoint(e.b);
            } else {
                // ray
                if (e.hasA){
                    writePoint(e.a);
                }else if (e.hasB){
                    writePoint(e.b);
                }
            }
            writePoint(e.direction);
            writePoint(e.geomDirection);
        }

        // ===== site indices =====
        writeSize(r.siteIndices.size());

        for (size_t idx : r.siteIndices) {
            writeSize(idx);
        }
        writeBool(r.hasPivot);
    }

    out.close();

    if (!out) {
        std::cerr << "Error while writing file: " << filename << "\n";
        exit(1);
    }
}

// Function to modify the old Voronoi diagram structure to a new one
void modifyStructure(const mygal::Diagram<double>& diagram, Voronoi::NewDiagram& newDiagram) {

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

int main(int argc, char* argv[]) {
    // Default value
    std::string fileName = "Data/equilatero_norm.txt";

    // Valgrind doesn't work with SFML/Graphics!
    // so you need to put this variable to false to be able to run Valgrind analysis correctly
    bool visualize = true;

    bool minKnapsack = true;

    bool save_final = false;

    bool save_each_image = false;

    bool save_diagram = false;

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
        }else if (arg=="--save_final"){
            if (i +1 < argc){
                std::string val = argv[i + 1];
                if (val == "0") save_final = false;
                else if (val=="1") save_final = true;
                else {
                    std::cerr << "Invalid value for --save-final: " << val << "\n";
                    return 1;
                }
                if (visualize==false && save_final==true){
                    std::cerr << "Cannot save final image if visualization is disabled.\n";
                    return 1;
                }
                i++;
            }
        }else if (arg=="--save_diagram"){
            if (i +1 < argc){
                std::string val = argv[i + 1];
                if (val == "0") save_diagram = false;
                else if (val=="1") save_diagram = true;
                else {
                    std::cerr << "Invalid value for --save-diagram: " << val << "\n";
                    return 1;
                }
                i++;
            }
        }else if (arg=="--save_each_step"){
            if (i +1 < argc){
                std::string val = argv[i + 1];
                if (val == "0") {save_each_image = false;}
                else if (val=="1") {
                    save_each_image = true;
                    save_final = false; // if we save each step we already save the final image
                }
                else {
                    std::cerr << "Invalid value for --save_each_step: " << val << "\n";
                    return 1;
                }
                if (visualize==false && save_each_image==true){
                    std::cerr << "Cannot save images of each step if visualization is disabled.\n";
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
    
    // Print points index and coordinates
    // std::cout << "Points and weights:\n";
    // for (size_t i = 0; i < points_with_weights.size(); ++i) {
    //     std::cout << "Index: " << i << ", Point: (" << points_with_weights[i].first.x << ", " << points_with_weights[i].first.y << "), Weight: " << points_with_weights[i].second << "\n";
    // }

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

    // std::cout << "MODIFIED STRUCTURE\n";
    // auto& fs = newDiagram.getFaces();
    // for (const auto& face : fs) {
    //     std::cout << *face <<"\n";
    //     Voronoi::NewDiagram::HalfEdgePtr x = face->firstEdge;
    //     do {
    //         std::cout << *x << "\n";
    //         x = x->next;
    //     } while (x != face->firstEdge);
    // }

    if (minKnapsack){
        //Construct the min-knapsack Voronoi diagram
        faces = build_minKnapsack(newDiagram, points_with_weights, capacity, save_each_image, fileName);
    }
    
    // std::cout << "Min Knapsack Voronoi Diagram\n";
    auto& fs = faces;
    for (const auto& face : fs) {
        if (face==nullptr) continue;
        std::cout << *face <<"\n";
    //     Voronoi::NewDiagram::HalfEdgePtr x = face->firstEdge;
    //     do {
    //         if (x == nullptr) {
    //             std::cout << "Encountered null half-edge!" << std::endl;
    //             break; // Exit the loop if x is null to prevent dereferencing
    //         }
    //         std::cout << *x << "\n";
    //         x = x->next;
    //     } while (x != face->firstEdge);
    }
    
    std::string fullPath = "Data/Saved_diagrams/" +  std::filesystem::path(fileName).stem().string() + ".bin";
    if (save_diagram){
        saveRegions(faces, fullPath);
    }

    // Visualize the diagram
    if (visualize==true){
        visualize_diagram(faces, points_with_weights, save_final, -1, fileName);
    }

    cleanupDiagram(newDiagram);

        
    return 0;
}
 

// This is the old version that DIDNT check for nullpointers but assumed that the diagram was just correct


// void initEdgePointsVis(Voronoi::NewDiagram::HalfEdgePtr& h, sf::VertexArray& line,
//     const std::vector<std::pair<Point2D,double>>& points) {
//     if (!h->head->infinite && !h->twin->head->infinite) {
//         // Both vertices defined, draw line between them
//         line[0].position = sf::Vector2f(h->head->point.x, h->head->point.y);
//         line[1].position = sf::Vector2f(h->twin->head->point.x , h->twin->head->point.y);
//     }
//     else if (!h->head->infinite) {
//         // One vertex is defined, project the other
//         line[0].position = sf::Vector2f(h->head->point.x, h->head->point.y);
//         Point2D norm = (points[h->twin->label->index].first - points[h->label->index].first).normalized().getRotated90CCW();
//         line[1].position = sf::Vector2f((line[0].position.x + norm.x * 1000), (line[0].position.y + norm.y * 1000));
//     }
//     else if (!h->twin->head->infinite) {
//         // Only the twin's vertex is defined, project the other
//         line[0].position = sf::Vector2f(h->twin->head->point.x , h->twin->head->point.y);
//         Point2D norm = (points[h->label->index].first - points[h->twin->label->index].first).normalized().getRotated90CCW();
//         line[1].position = sf::Vector2f((line[0].position.x + norm.x * 1000), (line[0].position.y + norm.y * 1000));
//     }
//     else {
//         // No vertices defined, project both points
//         Point2D p1 = points[h->twin->label->index].first, p2 = points[h->label->index].first;
//         Point2D norm = (p1 - p2).normalized().getRotated90CCW();
//         Point2D c = 0.5 * (p1 + p2);
//         line[0].position = sf::Vector2f((c.x + norm.x * 1000), (c.y + norm.y * 1000));
//         line[1].position = sf::Vector2f((c.x - norm.x * 1000), (c.y - norm.y * 1000));
//     }


//     // Set the color of the line
//     line[0].color = sf::Color::Green;
//     line[1].color = sf::Color::Green;
// }
