// maxflow.hpp -- Stage 3 of the pipeline: feasible supply-to-demand allocation.
#ifndef MAXFLOW_HPP
#define MAXFLOW_HPP

#include <vector>

namespace bf {

// Edmonds-Karp: Ford-Fulkerson with BFS augmenting-path selection.
// Complexity: O(V * E^2), independent of capacity magnitudes -- which matters
// here because blood unit counts are data-dependent.
//
// Edges are stored in pairs (e, e^1) so that the residual of edge i is i^1.
class MaxFlow {
public:
    explicit MaxFlow(int n);

    // Adds a directed edge u->v with the given capacity and returns the index
    // of the forward edge, so the caller can read back the flow later.
    int addEdge(int u, int v, int capacity);

    // Runs Edmonds-Karp and returns the value of the maximum flow.
    int run(int source, int sink);

    // Flow currently pushed along the forward edge with the given index.
    int flowOn(int edgeIndex) const;

private:
    struct Edge { int to; int cap; int flow; };

    bool bfs(int s, int t, std::vector<int>& parentEdge);

    int                            n_;
    std::vector<Edge>              edges_;
    std::vector<std::vector<int>>  adj_;   // adj_[v] = indices into edges_
};

} // namespace bf

#endif // MAXFLOW_HPP
