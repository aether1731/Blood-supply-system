// graph.hpp -- Stage 1 of the pipeline: shortest delivery routes.
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "model.hpp"
#include <vector>
#include <limits>

namespace bf {

const double INF_TIME = std::numeric_limits<double>::infinity();

// Result of one single-source shortest-path run.
struct ShortestPaths {
    std::vector<double> dist;    // dist[v] = minimum travel time from the source
    std::vector<int>    parent;  // parent[v] = predecessor of v on that path, -1 if none
};

// Dijkstra's algorithm on a non-negative weighted graph, implemented with a
// binary-heap priority queue. Complexity: O((V + E) log V).
//
// Travel times are never negative, which is exactly the precondition Dijkstra
// needs; that is why it is preferred here over Bellman-Ford.
ShortestPaths dijkstra(const std::vector<std::vector<Road>>& adj, int source);

// Rebuild the node sequence source -> ... -> target from a parent array.
// Returns an empty vector if the target is unreachable.
std::vector<int> reconstructPath(const ShortestPaths& sp, int source, int target);

} // namespace bf

#endif // GRAPH_HPP
