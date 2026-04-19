[19/04/2026 17:29] Vale: # MinKnapsack algorithm

This file contains all the decisions taken to implement the algorithm and all the problems that I met along the way.

## Diagram data structure

In the original structure the halfedges are connected in anti-clockwise sense, but we need a clockwise sense to align with the algorithm: thus we assign the prev halfedge as next and we assign the origin vertex as head and the destination vertex as tail.

I need each halfedge to point to the previous one, the following one and especially its twin. This means that I have to be very careful with pointers!

I had many problems with memory but it seems that, by freeing the whole diagram at the end of the main function, most of my problems disappeared.
For difficult diagrams I have again memory leaks.
I should fix the first iteration of all diagrams and then pass to the subsequent iterations.

For now it works only for simple regions up to region ord order 2.

One important bug: if I use & I am not modifying a copy but also the original object! Be careful!

I still have many memory leaks!!

Next thing to fix: I have to follow the example and assure that the special vertices are (1) correctly constructed (2) correctly iterated (3) correctly update connections AND region

I also need to understand if I need to keep regions update or not

IMPORTANT: I had to make the special vertices be checked two times. Otherwise I risk to merge together wrong vertices before the right ones. I also start merging from the circumcenter that is closest to the border. Maybe there is a way to avoid merging the wrong vertices before time? To be researched...

## Current problem to solve:
For now the algorithm seems to work for small-order regions but breaks with higher-order regions!
Since it breaks both with Min-Knapsack diagrams and k-order diagrams this problem seem to be connected to how I divide the regions and not specifically to some Min-Knapsack property.

Objective: I should for sure be able to recover a k-order diagram for any order! Most probable correction is to use the heap to be sure to divide the regions with multiple labels in the correct order. I also should correct the computation of the distance since it seems now that the plus distance of a vertex and the minus distance of the subsequent vertex do not concide, which is clearly not possible! I should also reconsider the check for the point to be inside the diagram (but maybe by starting always from the most close ones that outcome is avoided too). I should also be careful with the vertex on the infinite, should it always be the last to be connected or not? What did I wrote about it in the thesis?

Possible idea too: use CGAL library to check how a correct order should be. I could take the 100 points example, make each point weight 2 so that I need exactly 50 points and check if the 50-order diagram is actually computed.
[19/04/2026 17:29] Vale: ## What should happen for each example
- equilatero_norm &#8594; 3 points, I need exactly 2 to satisfy the request. I expect to see all regions of order 2.
- four_points &#8594; 4 points, I need exactly 2 to satisfy the request. I expect to see all regions of order 2.
- example_3 &#8594; 3 points, only one is able to satisfy by itself (the one in the bottom part), for all others I need multiple points. I expect 1 region of order 1 and all others of order 2. Since the points together have NOT the same weight I want to keep their order.
- example_4 &#8594; 3 points, only one is able to satisfy by itself (the one in the bottom part), for all others I need multiple points. I expect 1 region of order 1 and all others of order 2. Since the points together have the same weight I want to forget their order and fuse them together.
- hardest_one &#8594; 10 points, 9 of them satisfy the request by themself, while the central one needs exactly one of the others. I expect 9 regions of order 1 and the central region divided in 9 parts. (why hardest? because the iterative decomposition of the region needs 2 pass instead of just 1!)
- degenerate_1 &#8594; 3 collinear points, at least 2 or three of them are needed to satisfy the request. I don't know how it should be handled!!
- example_1 &#8594; 6 points, I need at least regions of 2 points to satisfy the request. I expect a mix of regions from (possibly) 2 to (possibly) 5
- example_2 &#8594; same 6 points, I need at least regions of 3 points to satisfy the request. I expect a mix of regions from (possibly) 3 to (possibly) 6
- Lee_1 &#8594; example from Lee paper: 16 points, I need exactly one to satisfy the request. I expect no iteration of minknapsack and just the first order diagram.
- Lee_2 &#8594;  example from Lee paper: 16 points, I need exactly two to satisfy the request. I expect all regions of order 2.
- Lee_fig2 &#8594; example from Lee paper: 8 points, I need all of them to satisfy the request. I expect one region with all of them 

## Temporary debugging prints:
To print the Voronoi diagram output of Fortune algorithm:
auto& hs = diagram.getHalfEdges();
for (const auto& he : hs) {
    std::cout << "Head: ";
    if (he.destination == nullptr) {
        std::cout << "infinite vertex\n";
    }
    else {
        std::cout << he.destination->point << "\n";
    }
    std::cout << "Tail: ";
    if (he.origin == nullptr) {
        std::cout << "infinite vertex\n";
    }
    else {
        std::cout << he.origin->point << "\n";
    }
}
To print the Voronoi diagram after the "translation":
std::cout << "MODIFIED STRUCTURE\n";
auto& fs = newDiagram.getFaces();
for (const auto& face : fs) {
    Voronoi::NewDiagram::HalfEdgePtr x = face->firstEdge;
    do {
        std::cout << "Head: ";
        if (x->head->infinite != true) {
            std::cout << x->head->point << "\n";
        }else {
            std::cout << "infinite vertex\n";
        }
        std::cout << "Tail: ";
        if (x->tail->infinite != true) {
            std::cout << x->tail->point << "\n";
        }else {
            std::cout << "infinite vertex\n";
        }
        x = x->next;
    } while (x != face->firstEdge);
}



## Compile commands
g++ -std=c++17 -IMyGAL/include Main.cpp MinKnapsack.cpp Point2D.cpp -o MyProgram -lsfml-graphics -lsfml-window -lsfml-system; ./MyProgram

GDB debugger compilation/execution:
g++ -g -std=c++17 -IMyGAL/include Main.cpp MinKnapsack.cpp Point2D.cpp -o MyProgram -lsfml-graphics -lsfml-window -lsfml-system; gdb ./MyProgram

run
bt


Valgrind compilation/execution:

g++ -g -fno-omit-frame-pointer -IMyGAL/include Main.cpp MinKnapsack.cpp Point2D.cpp -o MyProgram -lsfml-graphics -lsfml-window -lsfml-system; valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./MyProgram 


CGAL comparison
g++ main.cpp -lCGAL -lgmp -lmpfr -O3 -std=c++17

./a.out 3


# To generate points in python
import random

EPS = 1e-9

def are_cocircular(p1, p2, p3, p4):
    # Determinant test for cocircularity
    # | x  y  x²+y²  1 |
    # determinant == 0 → cocircular
[19/04/2026 17:29] Vale: def row(p):
        x, y = p
        return [x, y, x*x + y*y, 1]

    m = [row(p1), row(p2), row(p3), row(p4)]

    # Compute determinant (expanded manually for 4x4)
    import numpy as np
    return abs(np.linalg.det(m)) < EPS


def is_valid(new_point, existing_points):
    n = len(existing_points)
    if n < 3:
        return True

    # Check all triples with new point
    for i in range(n):
        for j in range(i+1, n):
            for k in range(j+1, n):
                if are_cocircular(existing_points[i],
                                  existing_points[j],
                                  existing_points[k],
                                  new_point):
                    return False
    return True


def generate_points(num_points):
    points = []

    while len(points) < num_points:
        x = random.random()
        y = random.random()
        candidate = (x, y)

        if is_valid(candidate, points):
            points.append(candidate)

    return points


#This generates diagrams where I need regions of intermediate dimension
def generate_file(filename="points.txt", num_points=100):
    # Target so ~50 points needed
    target = random.randint(400, 700)

    min_w, max_w = 5, 15

    coords = generate_points(num_points)

    points = []
    for (x, y) in coords:
        w = random.randint(min_w, max_w)
        points.append((x, y, w))

    # Estimate how many points needed to reach target
    sorted_weights = sorted([p[2] for p in points], reverse=True)
    partial_sum = 0
    count = 0
    for w in sorted_weights:
        partial_sum += w
        count += 1
        if partial_sum >= target:
            break

    print(f"Target: {target}")
    print(f"Points needed (greedy estimate): ~{count}")

    with open(filename, "w") as f:
        f.write(f"{target}\n")
        f.write(f"{num_points}\n")
        for x, y, w in points:
            f.write(f"{x:.6f} {y:.6f} {w}\n")


#This generates diagrams where I need exactly one region with all points (so i need to pass through many orders)
def generate_file(filename="points.txt", num_points=100):
    # Target so ~50 points needed
    target = num_points


    coords = generate_points(num_points)

    points = []
    for (x, y) in coords:
        points.append((x, y, 1.0))

    with open(filename, "w") as f:
        f.write(f"{target}\n")
        f.write(f"{num_points}\n")
        for x, y, w in points:
            f.write(f"{x:.6f} {y:.6f} {w}\n")


# To check cocircularity

import itertools

def read_points(filename):
    points = []
    with open(filename, 'r') as f:
        lines = f.readlines()
        
        # Skip first two lines (metadata)
        for line in lines[2:]:
            parts = line.strip().split()
            if len(parts) >= 2:
                x = float(parts[0])
                y = float(parts[1])
                points.append((x, y))
    
    return points


def determinant4(a, b, c, d):
    # Each point: (x, y)
    def row(p):
        x, y = p
        return [x, y, x*x + y*y, 1]
    
    mat = [row(a), row(b), row(c), row(d)]
    
    # Compute determinant manually (expansion or using helper)
    import numpy as np
    return np.linalg.det(mat)


def are_cocircular(a, b, c, d, eps=1e-7):
    det = determinant4(a, b, c, d)
    return abs(det) < eps


def check_no_cocircular(points):
    for quad in itertools.combinations(points, 4):
        if are_cocircular(*quad):
            print("Found cocircular points:", quad)
            return False
    return True


if name == "main":
    filename = "points.txt"
    points = read_points(filename)
    
    print(f"Loaded {len(points)} points")
    
    if check_no_cocircular(points):
        print("OK! No four cocircular points found")
    else:
        print("NO! There exist four cocircular points")