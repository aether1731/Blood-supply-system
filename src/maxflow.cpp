#include "maxflow.hpp"
#include <queue>
#include <algorithm>
#include <limits>

namespace bf {

MaxFlow::MaxFlow(int n) : n_(n), adj_(n) {}

int MaxFlow::addEdge(int u, int v, int capacity) {
    int idx = static_cast<int>(edges_.size());
    edges_.push_back({v, capacity, 0});   // forward edge
    edges_.push_back({u, 0, 0});          // residual (reverse) edge
    adj_[u].push_back(idx);
    adj_[v].push_back(idx + 1);
    return idx;
}

int MaxFlow::flowOn(int edgeIndex) const {
    return edges_[edgeIndex].flow;
}

// Breadth-first search for the augmenting path with the fewest edges.
// Choosing the shortest augmenting path (rather than any path, as in plain
// Ford-Fulkerson) is what bounds the number of augmentations by O(V * E).
bool MaxFlow::bfs(int s, int t, std::vector<int>& parentEdge) {
    parentEdge.assign(n_, -1);
    std::vector<bool> seen(n_, false);
    std::queue<int> q;
    q.push(s);
    seen[s] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (int idx : adj_[u]) {
            const Edge& e = edges_[idx];
            int residual = e.cap - e.flow;
            if (residual > 0 && !seen[e.to]) {
                seen[e.to] = true;
                parentEdge[e.to] = idx;
                if (e.to == t) return true;
                q.push(e.to);
            }
        }
    }
    return false;
}

int MaxFlow::run(int source, int sink) {
    int total = 0;
    std::vector<int> parentEdge;

    while (bfs(source, sink, parentEdge)) {
        // Bottleneck: the smallest residual capacity along the found path.
        int bottleneck = std::numeric_limits<int>::max();
        for (int v = sink; v != source; ) {
            int idx = parentEdge[v];
            bottleneck = std::min(bottleneck, edges_[idx].cap - edges_[idx].flow);
            v = edges_[idx ^ 1].to;
        }
        // Push the bottleneck along the path, updating residuals.
        for (int v = sink; v != source; ) {
            int idx = parentEdge[v];
            edges_[idx].flow     += bottleneck;
            edges_[idx ^ 1].flow -= bottleneck;
            v = edges_[idx ^ 1].to;
        }
        total += bottleneck;
    }
    return total;
}

} // namespace bf
