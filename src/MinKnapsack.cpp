#include <memory>
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <limits>
#include "MinKnapsack.h"
#include "Point2D.h"
#include "UnionFind.cpp"
#include "Diagram_visualization.h"

#define CIRCLE_CENTER_EPSILON 1.0e-7

long nR = 0;
double total;

enum direction{
    PLUS,
    MINUS,
    EQUAL
};
std::ostream& operator<<(std::ostream& out, const direction value){
    switch (value){
        case PLUS:
            out << "PLUS";
            break;
        case MINUS:
            out << "MINUS";
            break;
        case EQUAL:
            out << "EQUAL";
            break;
        default:
            out << "UNKNOWN_DIRECTION";
            break;
    }
    return out;
}

struct Special_vertex {
    Voronoi::NewDiagram::HalfEdgePtr edgein;
    Voronoi::NewDiagram::HalfEdgePtr edgeout;
    Voronoi::NewDiagram::FacePtr reg; //region at the right of the pending edge
    Voronoi::NewDiagram::SitePtr eti_minus;
    Voronoi::NewDiagram::SitePtr eti_plus;
    double d_minus;
    double d_plus;
    direction direz;

    friend std::ostream& operator<<(std::ostream& os, const Special_vertex& f) {
    os << "Special vertex with direction: " << f.direz << "\n";

    if (f.eti_minus)
        os << "eti_minus: " << f.eti_minus->index;
    else
        os << "eti_minus: NULL";

    if (f.eti_plus)
        os << ", eti_plus: " << f.eti_plus->index;
    else
        os << ", eti_plus: NULL";

    if (f.reg)
        os << ", region ID on the right: " << f.reg->ID;
    else
        os << ", region: NULL";

    if (f.edgein != nullptr) {
        os << ", edge-in:\n" << *(f.edgein) << "\n";
    }
    else {
        os << ", edge-in: null \n";
    }
    if(f.edgeout!=nullptr){
        os << ", edge-out:\n" << *f.edgeout;
    }else{
        os << ", edge-out null\n";
    }
    os << ", d_minus: " << f.d_minus;
    os << ", d_plus: " << f.d_plus << "\n";
    return os;
} 

};

// Functions to help delete references
void clearSpecialVertexVector(std::vector<Special_vertex>& vec) {
    for (auto& sv : vec) {
        // --- Clean up edgein ---
        if (sv.edgein) {
            sv.edgein->twin.reset();
            sv.edgein->next.reset();
            sv.edgein->region.reset();
            sv.edgein->tail.reset();
            sv.edgein->head.reset();
        }

        // --- Clean up edgeout ---
        if (sv.edgeout) {
            sv.edgeout->twin.reset();
            sv.edgeout->next.reset();
            sv.edgeout->region.reset();
            sv.edgeout->tail.reset();
            sv.edgeout->head.reset();
        }

        // --- Clean up region ---
        if (sv.reg) {
            // Optionally reset edges inside region if needed
            sv.reg.reset();
        }

        // --- Clean up site pointers ---
        sv.eti_minus.reset();
        sv.eti_plus.reset();
    }

    // Finally clear the vector itself
    vec.clear();
}

void clearHalfEdgeVector(std::vector<Voronoi::NewDiagram::HalfEdgePtr>& vec) {
    for (auto& he : vec) {
        if (he) {
            // Break cycles
            he->twin.reset();
            he->next.reset();
            he->region.reset();
            he->tail.reset();
            he->head.reset();
            he->label.reset();
        }
    }
    vec.clear();
}

void clearFaceVector(std::vector<Voronoi::NewDiagram::FacePtr>& vec) {
    // Faces themselves don't own edges or other cycles.
    // Just reset the shared_ptrs.
    for (auto& f : vec) {
        f.reset();
    }
    vec.clear();
}

// Function to compare sets (I don't care about order of sites, just that they are the same ones!)
bool same_sites(const std::vector<Voronoi::NewDiagram::SitePtr>& a, const std::vector<Voronoi::NewDiagram::SitePtr>& b) {
    if (a.size() != b.size()) return false;

    // make copies to sort
    std::vector<Voronoi::NewDiagram::SitePtr> sorted_a = a;
    std::vector<Voronoi::NewDiagram::SitePtr> sorted_b = b;

    auto cmp = [](const Voronoi::NewDiagram::SitePtr& s1,
                  const Voronoi::NewDiagram::SitePtr& s2) {
        return s1->index < s2->index;
    };

    std::sort(sorted_a.begin(), sorted_a.end(), cmp);
    std::sort(sorted_b.begin(), sorted_b.end(), cmp);

    for (size_t i = 0; i < sorted_a.size(); ++i) {
        if (sorted_a[i]->index != sorted_b[i]->index) return false;
    }

    return true;
}

Voronoi::NewDiagram::FacePtr createRegion(std::list<Voronoi::NewDiagram::FacePtr>& R,Voronoi::NewDiagram::FacePtr&  r, Voronoi::NewDiagram::SitePtr lambda, Voronoi::NewDiagram::HalfEdgePtr e) {
    nR++;
    Voronoi::NewDiagram::FacePtr t_ptr = std::make_shared<Voronoi::NewDiagram::Face>();
    auto& t = *t_ptr;
    t.ID = nR;
    t.flag = 1;
    t.firstEdge = e;
    if ((r->weight + lambda->capacity)>total) {
        t.sites = r->sites;
        t.weight = r->weight + lambda->capacity; //TODO: i changed this... it doesn't really sense to keep the pivot weight separate
        t.pivot = lambda;
    }
    else {
        t.sites = r->sites;
        t.sites.push_back(lambda);
        t.weight = r->weight+lambda->capacity;
        t.pivot = nullptr;
    }
    R.push_back(t_ptr);
    std::cout << "I added the region: " << (*(R.back())).ID <<"\n";
    return t_ptr;
}

void createPendingEdge(Voronoi::NewDiagram::HalfEdgePtr e, Voronoi::NewDiagram::HalfEdgePtr f) {
    auto a = std::make_shared<Voronoi::NewDiagram::HalfEdge>();
    auto b = std::make_shared<Voronoi::NewDiagram::HalfEdge>();
    a->tail = e->head;
    a->head = Voronoi::NewDiagram::Vertex::getNullVertex(); //??? when i put a real vertex i have to be careful to not change the value of the static vertex!!!!!!!!!!!!
    b->tail = Voronoi::NewDiagram::Vertex::getNullVertex(); //???
    b->head = f->tail;
    // TODO check if this makes sense: I differentiate if it's a pending edge which creates a new internal region or not
    if (e->twin->label == f->twin->label){
        a->label = e->label;
        b->label = f->label;
    }else{
        a->label = e->twin->label;
        b->label = f->twin->label;
    }
    a->twin = b;
    b->twin = a;
    b->next = f;
    e->next = a;
}

Point2D circumcenter(Point2D p1, Point2D p2, Point2D p3) {
    Point2D u1 = (p1 - p2).normalized(), u2 = (p3 - p2).normalized();

    double cross = crossProduct(u1, u2);

    // check if vectors are collinear
    if (fabs(cross) < CIRCLE_CENTER_EPSILON) {
        return false;
    }

    // get cental points
    Point2D pc1 = 0.5 * (p1 + p2), pc2 = 0.5 * (p2 + p3);

    // get free components
    double b1 = dotProduct(u1, pc1), b2 = dotProduct(u2, pc2);

    Point2D center;

    // calculate the center of a circle
    center.x = (b1 * u2.y - b2 * u1.y) / cross;
    center.y = (u1.x * b2 - u2.x * b1) / cross;

    return center;
}

Point2D circumcenter(std::vector<Voronoi::NewDiagram::SitePtr> s) {
    return circumcenter(s.at(0)->point, s.at(1)->point, s.at(2)->point);
}

double dist(Point2D p1, Point2D p2) {
    return sqrt(pow(p1.x - p2.x,2) + pow(p1.y - p2.y,2));
}

bool pointInsideRegion(Point2D p, Point2D p1, Point2D p2) {
    return (((p.x - p1.x) * (p2.y - p1.y)) 
        >= ((p.y - p1.y) * (p2.x - p1.x)));
}

void chooseDirection(Special_vertex* r) {
    r->direz = EQUAL;
    const double eps = 1e-9;  // tolerance for floating-point comparison

    if (r->d_minus != 0 && r->d_plus != 0)
    {
        // Same sign → reciprocal comparison flips direction
        if ((r->d_minus > 0 && r->d_plus > 0) ||
            (r->d_minus < 0 && r->d_plus < 0))
        {
            if (std::fabs(r->d_minus - r->d_plus) < eps)
            {
                r->direz = EQUAL;
            }
            else if (r->d_minus < r->d_plus)
            {
                r->direz = MINUS;
            }
            else // r->d_minus > r->d_plus
            {
                r->direz = PLUS;
            }
        }
        // Different signs
        else
        {
            if (r->d_minus > 0)
                r->direz = MINUS;
            else
                r->direz = PLUS;
        }
    }
}

int circumcenterInside(size_t& r_index, std::vector<Special_vertex>& L, int E, bool open) {
    auto v_ptr = std::make_shared<Voronoi::NewDiagram::Vertex>();
    auto& v = *v_ptr;

    size_t next_index = (r_index + 1) % L.size();
    size_t next_next_index = (r_index + 2) % L.size();

    auto r = L[r_index]; 
    auto next = L[next_index]; 
    auto next_next = L[next_next_index]; 

    v.triplet = std::vector{ next.eti_minus, next.eti_plus, next_next.eti_plus };
    std::cout << "I am checking the circumcenter of the points: " << next.eti_minus->index << ", " << next.eti_plus->index << ", " << next_next.eti_plus->index << "\n";
    v.point = circumcenter(v.triplet);

    next.edgein->next->head = v_ptr;
    next.edgein->next->twin->tail = v_ptr;
    next_next.edgein->next->head = v_ptr;
    next_next.edgein->next->twin->tail = v_ptr;
    // Connect internally edges that now go to same vertex
    next_next.edgein->next->next = next.edgein->next->twin;

    //std::cout << "Open: "<< open;
    if (E == 3) {
        if (!open) {
            std::cout << "Last 3 labels in closed region\n";
            r.edgein->next->head = v_ptr;
            r.edgein->next->twin->tail = v_ptr;
        // TODO i am not sure these two are really needed
        next.edgein->next->next = r.edgein->next->twin;
        r.edgein->next->next = next_next.edgein->next->twin;
        }
        else {
            std::cout << "Last 3 labels in open region\n";
            createPendingEdge(next.edgein->next, next_next.edgein->next->twin);
            r.edgein->next = next.edgein->next->next->twin;
            next.edgein->next->next->next = r.edgeout;
            r.edgein->next->tail->infinite = true;
            r.edgein->next->twin->head->infinite = true;
        }

        // Delete 
        if (L.size() <= 3){
            L.clear();
        }else if (r_index + 2 < L.size()){
            // No wrap, normal erase
            L.erase(L.begin() + r_index, L.begin() + r_index + 3);
        }else{
            // Wrap-around case (like 2,0,1)
            size_t k = (r_index + 3) % L.size();

            L.erase(L.begin() + r_index, L.end());  // erase tail
            L.erase(L.begin(), L.begin() + k);      // erase head
        }
        E = 0;
    }else{
        std::cout << "Circumcenter found at subsequent special vertex\n";
        createPendingEdge(next.edgein->next, next_next.edgein->next->twin);

        Special_vertex p = Special_vertex{};
        p.edgein = next.edgein->next;
        p.edgeout = next_next.edgein->next->twin;
        p.eti_minus = p.edgein->label;
        p.eti_plus = p.edgeout->label;
        // TODO I added assignment of the region!!
        p.reg = p.edgein->region; // Which region should I assign? a lot of edges don't have a region...

        // TODO check if this is better-----------
        size_t n = L.size();
        size_t n1 = (r_index + 1) % n;
        size_t n2 = (r_index + 2) % n;

        // Decide insertion position BEFORE modifying L
        bool r_was_last = (r_index == n - 1);

        std::vector<size_t> rem = { n1, n2 };
        std::sort(rem.begin(), rem.end(), std::greater<size_t>());

        // erase and adjust r_index safely
        for (size_t i : rem) {
            L.erase(L.begin() + i);
            if (i < r_index) r_index--;
        }

        // If r is last, insert p at front, else after r!!
        // this assures that if the region is open then the last special vertex keeps being the infinite one TODO
        size_t p_index;
        if (r_was_last) {
            p_index = 0;
            r_index = L.size(); // explicitly fix r (L.size() and not L.size() - 1 because we still have to insert p)
        } else {
            p_index = r_index + 1;
        }

        L.insert(L.begin() + p_index, p);
        E--;

        Special_vertex& r_ref = L[r_index]; //TODO add to pseudocode
        Special_vertex& p_ref = L[p_index]; //TODO add to pseudocode
        Special_vertex& next_ref = L[(p_index + 1) % L.size()]; //TODO add to pseudocode
        
        std::cout << "R special vertex: " << r_ref << "\n";
        // Now i check the new possible circumcenter to choose the correct signs/directions for the new special vertex and its neighbors
        Point2D P = circumcenter(r_ref.eti_minus->point, p_ref.eti_minus->point, p_ref.eti_plus->point);
        std::cout << "Circumcenter between points: " << r_ref.eti_minus->index << ", " << p_ref.eti_minus->index << ", " << p_ref.eti_plus->index << "\n";
        std::cout << "Circumcenter at position: " << P << "\n";
        std::cout << "Check if inside w.r.t. line from: " << r_ref.edgein->head->point << " to " << r_ref.edgeout->head->point << "\n";
        std::cout << "Point inside region: " << pointInsideRegion(P, r_ref.edgein->head->point, r_ref.edgeout->head->point) << "\n";
        if (pointInsideRegion(P, p_ref.edgein->tail->point, p_ref.edgein->head->point)) { //TODO add to pseudocode/assure that this is correct
            r_ref.d_plus = dist(P, r_ref.edgeout->tail->point);
            p_ref.d_minus = dist(P, p_ref.edgein->head->point);
        }
        else {
            r_ref.d_plus = -dist(P, r_ref.edgeout->tail->point);
            p_ref.d_minus = -dist(P, p_ref.edgein->head->point);
        }

        Point2D P1 = circumcenter(p_ref.eti_minus->point, p_ref.eti_plus->point, next_ref.eti_plus->point);
        std::cout << "Circumcenter between points: " << p_ref.eti_minus->index << ", " << p_ref.eti_plus->index << ", " << next_ref.eti_plus->index << "\n";
        std::cout << "Circumcenter at position: " << P1 << "\n";
        std::cout << "Check if inside w.r.t. line from: " << p_ref.edgein->head->point << " to " << p_ref.edgeout->head->point << "\n";
        std::cout << "Point inside region: " << pointInsideRegion(P1, p_ref.edgein->head->point, p_ref.edgeout->head->point) << "\n";
        if (pointInsideRegion(P1, p_ref.edgein->head->point, p_ref.edgeout->head->point)) { // TODO check if this is correct
            p_ref.d_plus = dist(P1, p_ref.edgeout->tail->point);
            next_ref.d_minus = dist(P1, next_ref.edgein->head->point);
        }
        else {
            p_ref.d_plus = -dist(P1, p_ref.edgeout->tail->point);
            next_ref.d_minus = -dist(P1, next_ref.edgein->head->point);
        }
        chooseDirection(&r_ref);
        chooseDirection(&p_ref);
        chooseDirection(&next_ref);

        std::cout << "!!!!!!!!!!!!After inserting new special vertex, I have:\n";
        std::cout << "Current special vertex: " << r_ref << "\n";
        std::cout << "New special vertex: " << p_ref << "\n";
        std::cout << "Next special vertex: " << next_ref << "\n";
        std::cout << "End of list!!!!!!!!!!!!\n";
    }
    return E;
}

void partition(Voronoi::NewDiagram::FacePtr& r_ptr, std::list<Voronoi::NewDiagram::FacePtr>& R) {
    auto& r = *r_ptr;
    auto e = std::vector<Voronoi::NewDiagram::HalfEdgePtr>();
    auto lambda = std::vector<Voronoi::NewDiagram::SitePtr>();
    auto reg = std::vector<Voronoi::NewDiagram::FacePtr>();

    e.push_back(r.firstEdge);
    lambda.push_back(e.at(0)->twin->label);
    reg.push_back(createRegion(R, r_ptr, lambda.at(0), e.at(0)));

    while (e.at(0)->next != r.firstEdge && e.at(0)->next->twin->label == lambda.at(0)) {
        e.at(0) = e.at(0)->next;
    }

    // If there are never changes in the external labels then it's a special case: we don't need to partition the region but just to add it as it is (and I already added it with the previous createRegion!)
    if (e.at(0)->next->twin->label != lambda.at(0)) {
        lambda.push_back(e.at(0)->next->twin->label);
        e.push_back(e.at(0)->next);
        createPendingEdge(e.at(0), e.at(1));
        reg.push_back(createRegion(R, r_ptr, lambda.at(1), e.at(0)->next->twin));

        while (e.at(1)->next->twin->label == lambda.at(1)) {
            e.at(1) = (e.at(1))->next;
        }


        // Check if I just need to partition in two subregions
        if ((e.at(1))->next->twin->label == lambda.at(0)) {
            std::cout << "Partitioning region in 2 subregions\n";
            e.at(0)->next->head = e.at(1)->head;
            e.at(0)->next->twin->tail = e.at(1)->head;
            e.at(0)->next->next = e.at(1)->next;
            e.at(1)->next = e.at(0)->next->twin;
        }else{
            // Case where I need to partition in 3 or more subregions
            int k = 1;
            lambda.push_back(e.at(1)->next->twin->label);
            e.push_back(e.at(1)->next);
            createPendingEdge(e.at(1), e.at(2));
            bool open = false;

            // Create all pending edges
            do {
                reg.push_back(createRegion(R, r_ptr, lambda.at(k + 1), e.at(k)->next->twin));
                k++; // in the algorithm k goes from 1 to length, here it goes from 0 to length-1!!
                while (e.at(k)->next->twin->label == lambda.at(k)) {
                    e.at(k) = e.at(k)->next;
                }
                lambda.push_back(e.at(k)->next->twin->label);
                e.push_back(e.at(k)->next);

                if (!(e.at(k)->head->infinite)) {
                    createPendingEdge(e.at(k), e.back());
                }
                else {
                    open = true;
                }
            } while (lambda.back() != lambda.front());

            // Partition in 3 parts (first part)
            std::vector<Special_vertex> L{};
            // E is the number of special vertices left to examine
            int E = 0;
            for (int h = 0; h <= k; h++) {
                ++E;
                Special_vertex rv{};
                rv.eti_minus = lambda.at(h);
                rv.eti_plus = lambda.at((h+1) % (k+1)); // TODO add to pseudocode
                rv.reg = reg.at((h+1) % (k+1)); // TODO add to pseudocode 
                rv.edgein = e.at(h);

                if (rv.edgein->head->infinite) {
                    rv.edgeout = e.at(h)->next;
                }
                else {
                    rv.edgeout = e.at(h)->next->twin->next;
                }
                L.push_back(rv);
            }

            for (size_t l_iter = 0; l_iter < L.size(); ++l_iter) {
                size_t l_next = (l_iter + 1) % L.size();
                //std::cout << "Circumcenter between points: " << L[l_iter].eti_minus->index << ", " << L[l_iter].eti_plus->index << ", " << L[l_next].eti_plus->index << "\n";
                Point2D P = circumcenter(L[l_iter].eti_minus->point, L[l_iter].eti_plus->point, L[l_next].eti_plus->point);

                //std::cout << "Circumcenter at: " << P << "\n";
                Point2D firstPoint, secondPoint;
                // I added this so that the special vertex at infinite is correctly seen as inside the region TODO add to pseudocode
                if (!L[l_iter].edgein->head->infinite) {
                    firstPoint = L[l_iter].edgein->head->point;
                    if (L[l_next].edgein->head->infinite){
                        firstPoint = L[l_iter].edgein->twin->head->point;
                        secondPoint = L[l_iter].edgein->head->point;
                    }else{
                        secondPoint = L[l_next].edgein->head->point;
                    }
                }else{
                    firstPoint = L[l_next].edgein->head->point;
                    secondPoint = L[l_next].edgeout->head->point;
                }
                //std::cout << "Region defined by points: " << firstPoint << ", " << secondPoint << "\n";
                //std::cout << "Circumcenter inside region: " << pointInsideRegion(P, firstPoint, secondPoint) << "\n";
                if (pointInsideRegion(P, firstPoint, secondPoint)) {
                    if (!L[l_iter].edgeout->tail->infinite) {
                        L[l_iter].d_plus = dist(P, L[l_iter].edgeout->tail->point);
                    }
                    if (!L[l_next].edgeout->tail->infinite) {
                        L[l_next].d_minus = dist(P, L[l_next].edgeout->tail->point);
                    }
                    
                }
                else {
                    if (!L[l_iter].edgeout->tail->infinite) {
                        L[l_iter].d_plus = -dist(P, L[l_iter].edgeout->tail->point);
                    }
                    if (!L[l_next].edgeout->tail->infinite) {
                        L[l_next].d_minus = -dist(P, L[l_next].edgeout->tail->point);
                    }
                }
            }

            int index = 0;
            size_t l_last_iter = L.size() - 1; // TODO: why sometimes it does not enter in the if? i should check
            double min_dis = std::numeric_limits<double>::max();

            std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!Initial special vertices and their directions:\n";
            for (size_t l_iter = 0; l_iter < L.size(); ++l_iter) {
                auto& vertex = L.at(l_iter);
                chooseDirection(&vertex);
                std::cout << vertex <<"\n";
                auto& prev = L.at((l_iter + L.size() - 1) % L.size());
                if (vertex.direz== MINUS &&  prev.direz == PLUS && vertex.d_minus < min_dis) {
                    min_dis = vertex.d_minus;
                    l_last_iter = (l_iter + L.size() - 2) % L.size();
                }
            }
            std::cout << "End of list!!!!!!!!!!!!!!!!!!!!!!!!\n";

            // Partition in 3 parts (second part)
            // I keep iterating over the special vertices until I finish the partitions to create
            if (open) {
                l_last_iter = L.size() - 1;
            }
            //std::cout << "Last special vertex to examine: " << L.at(l_last_iter) << "\n";

            while(true) {
                //std::cout << ((l_last_iter + 2) % L.size()) << " " << L[(l_last_iter + 1) % L.size()].direz << " " << L[(l_last_iter + 2) % L.size()].direz << " " << L[(l_last_iter + 1) % L.size()].d_plus << "\n";


                // This condition skips all special vertices that cannot lead to a circumcenter inside
                //((((l_last_iter + 2) % L.size()) == 0) && open) || <- in this case (l_last_iter + 1) would be the last special vertex, which is infinite, so I skip it
                // TODO add to pseudocode
                while (((((l_last_iter + 2) % L.size()) == 0) && open) || (L[(l_last_iter + 1) % L.size()].direz == MINUS || L[(l_last_iter + 2) % L.size()].direz == PLUS || L[(l_last_iter + 1) % L.size()].d_plus < 0 && !open) || (E==3 && open && (l_last_iter != L.size() - 1))) {                        
                    l_last_iter = (l_last_iter + 1) % L.size();
                }

                std::cout << "I am examining vertex: " << L.at(l_last_iter) << "\n and I obtain conditions:\n";
                std::cout << "Direction of first special vertex: "<< L[(l_last_iter + 1) % L.size()].direz << "\nDirection of second special vertex: " << L[(l_last_iter + 2) % L.size()].direz << "\nDistance from first vertex to circumcenter: " << L[(l_last_iter + 1) % L.size()].d_plus << "\nFirst vertex is infinite: " << L[(l_last_iter + 1) % L.size()].edgein->head->infinite << "\n";

                std::cout << "Number of special vertices left to examine: " << E << "\n";
                std::cout << "open:" << open << "\n";

                // Check if there exists a circumcenter inside
                if (L[(l_last_iter + 1) % L.size()].direz != MINUS &&
                    L[(l_last_iter + 2) % L.size()].direz != PLUS &&
                    L[(l_last_iter + 1) % L.size()].d_plus > 0 &&
                    !(L[(l_last_iter + 1) % L.size()].edgein->head->infinite)) {
                    E = circumcenterInside(l_last_iter, L, E, open);
                }
                else if (open==true) { //TODO add to pseudocode
                    // CAREFUL: It should enter here ONLY if the region is open! Otherwise it should have found the last circumcenter and got to E=0 !!
                    // Maybe I should add a check here to be sure that the region is open
                    std::cout << "Circumcenter not found: connecting all edges to infinite \n";
                    for (size_t l_iter = 0; l_iter < L.size()-2; ++l_iter) {
                        L[(l_iter + 1) % L.size()].edgein->next->head = Voronoi::NewDiagram::Vertex::getNullVertex();
                        L[(l_iter + 1) % L.size()].edgein->next->head->infinite = true;
                        L[l_iter].edgein->next->twin->tail = Voronoi::NewDiagram::Vertex::getNullVertex();
                        L[l_iter].edgein->next->twin->tail->infinite = true;
                        L[(l_iter + 1) % L.size()].edgein->next->next = L[l_iter].edgein->next->twin;
                        //L[l_iter].reg->firstEdge = L[l_iter].edgein->next->twin; // This doesn't work now! and do I even really need it? i give firstedge afterwards...
                    }
                    int l_iter = L.size() - 2;
                    L[l_iter].edgein->next->twin->tail = Voronoi::NewDiagram::Vertex::getNullVertex();
                    L[l_iter].edgein->next->twin->tail->infinite = true;
                    L[(l_iter+1) % L.size()].edgein->next = L[l_iter].edgein->next->twin;
                    // L[l_iter].reg->firstEdge = L[l_iter].edgein->next->twin; // This doesn't work now! and do I even really need it? i give firstedge afterwards...
                    l_iter = L.size() - 1;
                    L[(l_iter+1) % L.size()].edgein->next->head = Voronoi::NewDiagram::Vertex::getNullVertex();
                    L[(l_iter+1) % L.size()].edgein->next->head->infinite = true;
                    L[(l_iter+1) % L.size()].edgein->next->next = L[l_iter].edgeout;
                    // I delete the list making sure to not leave dandling references
                    //clearSpecialVertexVector(L);
                    L.clear();
                    E = 0;
                }
                if (E==0){
                    std::cout << "Finished partitioning region!\n";
                    break;
                }
            }
        }
    }else{
        std::cout << "Added same region with one point more\n";
    }

}

std::list<Voronoi::NewDiagram::FacePtr> build_minKnapsack(Voronoi::NewDiagram& diagram, std::vector<std::pair<Point2D, double>>& points, double capacity) {
	std::list<Voronoi::NewDiagram::FacePtr>& R = diagram.getFaces();
    // these two are global variables because I use them inside other functions,
    // TODO remove global variables
    total = capacity;
    nR = R.size() - 1;
    std::list<Voronoi::NewDiagram::FacePtr>::iterator it = R.begin();
    long firstNew = R.size();
    // I keep examining regions until I arrive to the end of the list
    // If I comment the while I test a single iteration!
    while(it != R.end()){
        //visualize_diagram(R, points);
        firstNew = R.back()->ID + 1;
        std::list<Voronoi::NewDiagram::FacePtr>::iterator firstNewRegion = R.empty() ? R.end() : std::prev(R.end());
        // I partition all the new regions and add new ones to the end of the list
        while (it != R.end() && (*it)->ID < firstNew) {
            std::cout << "I examine the region: " << *(*it);
            std::cout << "Point: " << ((*it)->sites).at(0)->point << "\n";
            if ((*it)->flag == 1 && (*it)->pivot == nullptr && ((*it)->weight < capacity)) {
                std::cout << "I divide the region: " << (*it)->ID << "\n";
                partition(*it, R);
                (*it)->flag = 0;
                // I erase the current region and pass to the next (putting the flag to 0 instead of deleting right away to avoid invalidating the iterator)
                // TODO add to pseudocode (?)
            }
            ++it;
        }
        // If no new regions have been added I return right away!
        if (firstNew==(R.back()->ID + 1)){
            return R;
        }
        std::cout << "I partitioned all regions!\n";
        long lastNewBeforeMerge = nR;
        firstNewRegion = std::next(firstNewRegion); // I set it to point to the first new region
        it = firstNewRegion;

        // Create structure Union-Find
        UF unionFind = UF(lastNewBeforeMerge-((*firstNewRegion)->ID)+1);

        // All new edges created during the partitions must be assigned their region
        // I create a list of new edges (ListE)
        auto E = std::list<Voronoi::NewDiagram::HalfEdgePtr>();

        // I iterate over all new regions (it is already pointing to the first new region)
        while (it != R.end()){
            // I insert each new region in the Union-Find structure and each of their edges in the ListE
            // REMARK: this list will contain both new edges and old edges of the previous regions
            unionFind.add_UF(((*it)->ID)-(*firstNewRegion)->ID, *it); 
            Voronoi::NewDiagram::HalfEdgePtr e = (*it)->firstEdge;
            //std::cout << "Following all the edges of region " << (*it)->ID << "\n";
            auto newFirstEdge = e;
            do{
                e->region = (*it);
                //std::cout << "Next edge\n" << *e <<"\n";
                E.push_back(e);
                if (e->tail->infinite){
                    //TODO maybe there is a way to keep it correctly updated during the partitioning itself
                    // this is the correct firstedge! I save it to update it at the end of the traversing
                    newFirstEdge = e;
                }
                e = e->next;
            }while(e!=(*it)->firstEdge);
            (*it)->firstEdge = newFirstEdge;
            ++it;
        }
        //std::cout << "I added all new regions to the union find structure!\n";
        //std::cout << "Total edges in the graph: " << E.size() <<"\n";
        std::list<Voronoi::NewDiagram::HalfEdgePtr>::iterator e = E.begin();
        while (e != E.end()){
            // I iterate over ListE: if an edge is actually a new edge (divides different regions) then I take it out of ListE
            // This means that ListE will contain all edges to be eliminated
            // TODO add to pseudocode
            if(!same_sites((*e)->region->sites, (*e)->twin->region->sites) || (*e)->region->pivot!=nullptr || (*e)->twin->region->pivot!=nullptr){
                e = E.erase(e);
            }else{
                // If an edge is NOT a new edge it means its neighbouring regions are actually the same region: I sign it as to be eliminated and apply Union-Find
                (*e)->region->flag=0;
                unionFind.merge(unionFind.find((*e)->region->ID-(*firstNewRegion)->ID), unionFind.find((*e)->twin->region->ID-(*firstNewRegion)->ID)); //TODO merge together the regions
                ++e;
            }
        }
        std::cout << "I merged all regions that are actually the same!\n";


        for (int i =0; i<= (lastNewBeforeMerge-(*firstNewRegion)->ID);++i){
            // I iterate over all new regions and use Union-Find to find the first of all components that form a same new region
            //std::cout << "I examine element of the union find vector: " <<i <<" which is the region "<< *(unionFind.element(i).region) <<"\n";
            
            if (unionFind.find(i)==i && unionFind.element(i).next!=nullptr){
                //std::cout << "Index of the root: " << unionFind.find(i) << " which is the region " << unionFind.element(unionFind.find(i)).region->ID << ", next region in the same component is " << unionFind.element(unionFind.find(i)).next->region->ID <<"\n";
                auto* node = &unionFind.element(i);   // start at root
                Voronoi::NewDiagram::FacePtr r = node->region;
                Voronoi::NewDiagram::FacePtr t_ptr = std::make_shared<Voronoi::NewDiagram::Face>();
                nR++;
                t_ptr->ID = nR;
                t_ptr->flag = 1;
                t_ptr->weight = r->weight; 
                t_ptr->sites = r->sites; 
                Voronoi::NewDiagram::HalfEdgePtr newRegionFirstEdge = nullptr;
                while (node != nullptr) {
                    auto e = node->region->firstEdge;
                    do{
                        e->region = t_ptr;
                        // TODO add to pseudocode
                        // Reason I changed: I don't know the order in which I visit the subregions that form the fused region
                        if (newRegionFirstEdge == nullptr || (e->tail->infinite == true && !same_sites(e->region->sites, e->twin->region->sites))){
                            newRegionFirstEdge = e;
                        }
                        e = e->next;  
                    }while (e != node->region->firstEdge);
                    node = node->next;
                }
                // If a region is composed of many components, I create a new record for the first component region: this will represent the whole fused region
                // I insert this region in the list of regions ListR
                t_ptr->firstEdge = newRegionFirstEdge;
                //std::cout << *newRegionFirstEdge;
                R.push_back(t_ptr);
                //std::cout << "New merged region:\n" << *t_ptr <<"\n";
            }
        }
        
        //std::cout << "Number of edges to delete: " << E.size() <<"\n";
        // I iterate over ListE and update their connections so that, when they wil be eliminated, all other edges will be correctly connected
        e = E.begin();
        while (e != E.end()){
            auto f = (*e)->next->twin;
            while(f->next!=(*e)->twin && (*e)!=f){
                f = f->next->twin;
            }
            f->next = (*e)->next;
            ++e;
        }
        //std::cout << "I deleted all connections of useless edges!\n";

        // I delete all edges in ListE
        for (auto& edge : E) {
            diagram.deleteHalfEdge(edge);
        }
        E.clear();
        //std::cout << "I deleted all useless edges!\n";

        it = R.begin();
        // I delete regions that have been fused together
        while (it != R.end()) {
            if (!(*it)->flag) {
                if(it==firstNewRegion){
                    firstNewRegion = std::next(firstNewRegion); //TODO add to pseudocode
                }
                it = R.erase(it); // erase safely
            } else {
                ++it;
            }
        }
        it = firstNewRegion;
        //std::cout << "I deleted all useless regions!\n";
    }
    return R;
}
