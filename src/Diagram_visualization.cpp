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


void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, std::vector<std::pair<Point2D, double>>& points) {
    std::cout << "Visualizing diagram:\n";
    // Visualize the diagram
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

    }
}


void visualize_diagram(std::list<Voronoi::NewDiagram::FacePtr>& faces, const std::vector<std::pair<Point2D, double>>& points, bool saved, bool minKnapsack, std::string fileName){
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
            std::string suffix = minKnapsack ? "_MKVD" : "_base";   // change if needed
            std::string folder = "Images";
            std::string fullPath = folder + "/" +  std::filesystem::path(fileName).stem().string() + suffix + ".png";
            texture.copyToImage().saveToFile(fullPath);
            saved = false;
        }
    }
}
