# MinKnapsack algorithm

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

## What should happen for each example
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
```c++
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
```
To print the Voronoi diagram after the "translation":
```c++
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
```



## Compile commands
```bash
g++ -std=c++17 -IMyGAL/include Main.cpp MinKnapsack.cpp Point2D.cpp -o MyProgram -lsfml-graphics -lsfml-window -lsfml-system; ./MyProgram
```

**GDB debugger** compilation/execution:
```bash
g++ -g -std=c++17 -IMyGAL/include Main.cpp MinKnapsack.cpp Point2D.cpp -o MyProgram -lsfml-graphics -lsfml-window -lsfml-system; gdb ./MyProgram

run
bt

```

**Valgrind** compilation/execution:

```bash
g++ -g -fno-omit-frame-pointer -IMyGAL/include Main.cpp MinKnapsack.cpp Point2D.cpp -o MyProgram -lsfml-graphics -lsfml-window -lsfml-system; valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all ./MyProgram 
```


**CGAL comparison**
g++ main.cpp -lCGAL -lgmp -lmpfr -O3 -std=c++17

./a.out 3


