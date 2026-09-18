#include "graph.hpp"
#include <queue>
#include <algorithm>

namespace bf {

ShortestPaths dijkstra(const std::vector<std::vector<Road>>& adj, int source) {
    const int n = static_cast<int>(adj.size());
    ShortestPaths sp;
    sp.dist.assign(n, INF_TIME);
    sp.parent.assign(n, -1);

    // Min-heap of (tentative distance, node). The standard library gives a
    // max-heap by default, so the comparator is flipped with std::greater.
    using Entry = std::pair<double, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

    sp.dist[source] = 0.0;
    pq.push({0.0, source});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        // Lazy deletion: an outdated entry for u may still sit in the heap.
        if (d > sp.dist[u]) continue;

        for (const Road& e : adj[u]) {
            double nd = d + e.weight;
            if (nd < sp.dist[e.to]) {      // relaxation step
                sp.dist[e.to]   = nd;
                sp.parent[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }
    return sp;
}

std::vector<int> reconstructPath(const ShortestPaths& sp, int source, int target) {
    std::vector<int> path;
    if (target < 0 || target >= static_cast<int>(sp.dist.size())) return path;
    if (sp.dist[target] == INF_TIME) return path;   // unreachable

    for (int v = target; v != -1; v = sp.parent[v]) {
        path.push_back(v);
        if (v == source) break;
    }
    std::reverse(path.begin(), path.end());
    if (path.empty() || path.front() != source) return {};
    return path;
}

} // namespace bf
