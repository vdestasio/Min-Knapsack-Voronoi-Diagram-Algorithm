#include <SFML/Graphics.hpp>
#include <filesystem>
#include "Point2D.h"
#include "NewDiagram.h"
#include "Diagram_visualization.h"


// Function to draw a grid
void drawGrid(sf::RenderWindow& window, int gridSpacing){
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
void initEdgePointsVis(Voronoi::NewDiagram::HalfEdgePtr& h, sf::VertexArray& line, const std::vector<std::pair<Point2D,double>>& points){
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

void appendRegionEdges(const RegionData& r, sf::VertexArray& allEdges){
    for (const auto& e : r.boundary) {

        if (!e.isRay) {
            // --- segment ---
            allEdges.append(sf::Vertex(
                sf::Vector2f(e.a.x, e.a.y), sf::Color::Green));

            allEdges.append(sf::Vertex(
                sf::Vector2f(e.b.x, e.b.y), sf::Color::Green));
        }
        else {
            // --- ray ---
            Point2D origin = e.origin;
            Point2D dir = e.rayDirection;

            dir.normalize(); // safety

            double length = 100.0;
            Point2D end = origin + dir * length;

            allEdges.append(sf::Vertex(
                sf::Vector2f(origin.x, origin.y), sf::Color::Blue));

            allEdges.append(sf::Vertex(
                sf::Vector2f(end.x, end.y), sf::Color::Blue));
        }
    }
}

void visualize_diagram(std::vector<RegionData>& regions, std::vector<Point2D>& points){
    std::cout << "Visualizing regions:\n";

    sf::RenderWindow window(sf::VideoMode(800, 800), "Regions", sf::Style::Close);

    sf::View view;
    view.setCenter(0.5f, 0.5f);
    view.setSize(1.0f, -1.0f); // invert y
    window.setView(view);

    window.setVerticalSyncEnabled(false);

    // --- draw points ---
    float radius = 0.01f;
    std::vector<sf::CircleShape> shapes;
    for (const auto& point : points) {
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(radius, radius);
        shape.setPosition(point.x, point.y);
        shapes.push_back(shape);
    }
    // change color to last point
    if (!points.empty()) {
        shapes.back().setFillColor(sf::Color::Magenta);
    }

    // --- build all edges once ---
    sf::VertexArray allEdges(sf::Lines);

    for (const auto& r : regions) {
        appendRegionEdges(r, allEdges);
    }

    // --- render loop ---
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::Resized) {
                view.setSize(1.0f,
                    -1.0f * static_cast<float>(event.size.height) / event.size.width);
                window.setView(view);
            }
        }

        window.clear(sf::Color::White);

        drawGrid(window, 50);

        for (const auto& shape : shapes)
            window.draw(shape);

        window.draw(allEdges);

        window.display();
    }
}

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, std::vector<std::pair<Point2D, double>>& points) {
    visualize_diagram(faces, points, false, false, "");
}

void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, const std::vector<std::pair<Point2D, double>>& points, bool saved, int minKnapsack, std::string fileName){
    sf::RenderWindow window(sf::VideoMode(800, 800), "Plot Points",sf::Style::Close);

    auto size = window.getSize();
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    window.setPosition(sf::Vector2i(
        (desktop.width - size.x) / 2,
        (desktop.height - size.y) / 2
    ));
    
    bool dragging = false;
    sf::Vector2i lastMousePos;

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
    for (const auto& point : points) {
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
            initEdgePointsVis(edge, line, points);
            allEdges.append(line[0]);
            allEdges.append(line[1]);
            // TODO: sometimes the next is nullpointer, understand WHY it's nullpointer!!!
            edge = edge->next;
        } while (edge && edge != face->firstEdge);
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    dragging = true;
                    lastMousePos = sf::Mouse::getPosition(window);
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    dragging = false;
                }
            }

            if (event.type == sf::Event::MouseMoved && dragging) {
                sf::Vector2i newMousePos = sf::Mouse::getPosition(window);

                // convert pixel movement to world movement
                sf::Vector2f worldPos1 = window.mapPixelToCoords(lastMousePos);
                sf::Vector2f worldPos2 = window.mapPixelToCoords(newMousePos);

                sf::Vector2f delta = worldPos1 - worldPos2;

                view.move(delta);
                window.setView(view);

                lastMousePos = newMousePos;
            }

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

        if (saved) {
            sf::Texture texture;
            texture.create(window.getSize().x, window.getSize().y);
            texture.update(window);
            std::string suffix;
            if (minKnapsack==0){
                suffix="_base";    
            }else if(minKnapsack==-1){
                suffix="_MKVD";
            }else{
                suffix="_" + std::to_string(minKnapsack) + "_order";
            }
            std::string folder = "Images/" + std::filesystem::path(fileName).stem().string();
            if (!std::filesystem::exists(folder)) {
                std::filesystem::create_directory(folder);
            }
            std::string fullPath = folder + "/diagram" + suffix + ".png";
            texture.copyToImage().saveToFile(fullPath);
            saved = false;
        }
    }
}
