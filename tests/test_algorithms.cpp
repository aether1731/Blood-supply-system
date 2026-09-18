// test_algorithms.cpp -- correctness checks against hand-computed answers.
// Build:  make test
#include "../src/graph.hpp"
#include "../src/maxflow.hpp"
#include "../src/knapsack.hpp"
#include "../src/priority.hpp"
#include "../src/model.hpp"

#include <iostream>
#include <cmath>

using namespace bf;

static int failures = 0;

static void check(bool cond, const std::string& what) {
    std::cout << (cond ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!cond) ++failures;
}

int main() {
    std::cout << "Algorithm verification\n----------------------\n";

    // --- Dijkstra -----------------------------------------------------------
    // 0 --4-- 1 --3-- 3
    //  \--1-- 2 --1--/
    // Shortest 0->3 is 0-2-3 with cost 2, not 0-1-3 with cost 7.
    {
        std::vector<std::vector<Road>> adj(4);
        auto add = [&](int u, int v, double w) {
            adj[u].push_back({v, w});
            adj[v].push_back({u, w});
        };
        add(0, 1, 4); add(1, 3, 3); add(0, 2, 1); add(2, 3, 1);

        ShortestPaths sp = dijkstra(adj, 0);
        check(std::fabs(sp.dist[3] - 2.0) < 1e-9, "Dijkstra: distance 0->3 is 2");
        std::vector<int> path = reconstructPath(sp, 0, 3);
        check(path.size() == 3 && path[0] == 0 && path[1] == 2 && path[2] == 3,
              "Dijkstra: path 0->3 is 0-2-3");

        adj.push_back({});                      // isolated node 4
        ShortestPaths sp2 = dijkstra(adj, 0);
        check(sp2.dist[4] == INF_TIME, "Dijkstra: isolated node is unreachable");
    }

    // --- Edmonds-Karp -------------------------------------------------------
    // Classic CLRS-style network; maximum flow from 0 to 3 is 5.
    {
        MaxFlow mf(4);
        mf.addEdge(0, 1, 3);
        mf.addEdge(0, 2, 2);
        mf.addEdge(1, 2, 5);
        mf.addEdge(1, 3, 2);
        mf.addEdge(2, 3, 3);
        check(mf.run(0, 3) == 5, "Edmonds-Karp: max flow is 5");
    }
    {
        // Flow is bounded by the minimum cut, here the single edge of capacity 1.
        MaxFlow mf(3);
        mf.addEdge(0, 1, 100);
        mf.addEdge(1, 2, 1);
        check(mf.run(0, 2) == 1, "Edmonds-Karp: bottleneck edge caps the flow");
    }

    // --- Knapsack -----------------------------------------------------------
    // Weights 10/20/30, values 60/100/120, capacity 50 -> optimum 220 (items 1,2).
    {
        KnapsackResult r = knapsack({10, 20, 30}, {60, 100, 120}, 50);
        check(std::fabs(r.bestValue - 220.0) < 1e-9, "Knapsack: optimum value is 220");
        check(r.usedCapacity == 50, "Knapsack: uses the full 50 units of capacity");
        check(r.chosen.size() == 2 && r.chosen[0] == 1 && r.chosen[1] == 2,
              "Knapsack: selects items 1 and 2");
    }
    {
        KnapsackResult r = knapsack({40}, {99}, 30);   // item does not fit
        check(r.bestValue == 0.0 && r.chosen.empty(), "Knapsack: oversized item is rejected");
    }

    // --- Blood compatibility ------------------------------------------------
    check(compatible(O_NEG, AB_POS),  "Compatibility: O- donates to AB+");
    check(compatible(O_NEG, O_NEG),   "Compatibility: O- donates to O-");
    check(!compatible(A_POS, O_NEG),  "Compatibility: A+ cannot donate to O-");
    check(!compatible(AB_POS, A_POS), "Compatibility: AB+ cannot donate to A+");

    // --- Greedy triage ------------------------------------------------------
    // A CRITICAL request must outrank a MEDIUM one regardless of patient count.
    {
        PriorityWeights w;
        std::vector<Request> rs(2);
        rs[0].id = 1; rs[0].urgency = MEDIUM;   rs[0].criticalPatients = 50; rs[0].deadline = 60;
        rs[1].id = 2; rs[1].urgency = CRITICAL; rs[1].criticalPatients = 1;  rs[1].deadline = 60;
        scoreRequests(rs, w);
        std::vector<int> order = rankByPriority(rs);
        check(order[0] == 1, "Triage: CRITICAL outranks MEDIUM with many patients");
    }

    std::cout << "----------------------\n"
              << (failures ? "FAILURES: " : "All checks passed. Failures: ")
              << failures << "\n";
    return failures ? 1 : 0;
}
