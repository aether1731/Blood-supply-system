// main.cpp -- Emergency Blood Supply Distribution and Hospital Allocation System
// CSE 4403 (Algorithms) -- Assignment 2 (Quiz 4)
//
// Pipeline:
//   Stage 1  Dijkstra         -> fastest bank->hospital travel times and routes
//   Stage 2  Greedy triage    -> priority score and ranking of requests
//   Stage 3  Edmonds-Karp     -> feasible allocation of units, run in urgency tiers
//   Stage 4  Knapsack DP      -> loading of delivery vehicles under capacity limits
//   Stage 5  Dispatch report  -> routes, unmet demand, summary

#include "model.hpp"
#include "graph.hpp"
#include "priority.hpp"
#include "maxflow.hpp"
#include "knapsack.hpp"
#include "scenario.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

using namespace bf;

// ---------------------------------------------------------------------------
// Small printing helpers
// ---------------------------------------------------------------------------
static void rule(char c = '-', int n = 78) {
    std::cout << std::string(n, c) << "\n";
}

static void heading(const std::string& text) {
    std::cout << "\n";
    rule('=');
    std::cout << text << "\n";
    rule('=');
}

static std::string fmtTime(double minutes) {
    if (minutes == INF_TIME) return "unreachable";
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << minutes << " min";
    return os.str();
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    Scenario sc;

    if (argc > 1) {
        std::string err;
        if (!loadScenario(argv[1], sc, err)) {
            std::cerr << "Error reading scenario: " << err << "\n";
            return 1;
        }
        std::cout << "Loaded scenario from " << argv[1] << "\n";
    } else {
        sc = buildSampleScenario();
        std::cout << "No scenario file given; using the built-in sample.\n"
                  << "Usage: bloodflow [scenario_file]\n";
    }

    std::cout << "\nScenario: " << sc.title << "\n"
              << "Nodes: " << sc.nodes.size()
              << " | Blood banks: " << sc.banks.size()
              << " | Requests: " << sc.requests.size()
              << " | Vehicles: " << sc.vehicles.size() << "\n";

    // -----------------------------------------------------------------------
    // Stage 1: shortest delivery routes (Dijkstra, once per blood bank)
    // -----------------------------------------------------------------------
    heading("STAGE 1  Shortest delivery routes (Dijkstra)");

    std::vector<ShortestPaths> sp(sc.banks.size());
    for (size_t b = 0; b < sc.banks.size(); ++b)
        sp[b] = dijkstra(sc.adj, sc.banks[b].nodeId);

    std::cout << std::left << std::setw(28) << "Blood bank"
              << std::setw(28) << "Hospital"
              << std::setw(12) << "Travel time" << "Route\n";
    rule();

    for (size_t b = 0; b < sc.banks.size(); ++b) {
        for (const Node& n : sc.nodes) {
            if (n.type != HOSPITAL_NODE) continue;
            std::cout << std::left << std::setw(28) << sc.banks[b].name
                      << std::setw(28) << n.name
                      << std::setw(12) << fmtTime(sp[b].dist[n.id]);
            std::vector<int> path = reconstructPath(sp[b], sc.banks[b].nodeId, n.id);
            for (size_t i = 0; i < path.size(); ++i)
                std::cout << (i ? " -> " : "") << sc.nodes[path[i]].name;
            std::cout << "\n";
        }
    }

    // -----------------------------------------------------------------------
    // Stage 2: greedy triage
    // -----------------------------------------------------------------------
    heading("STAGE 2  Greedy triage ranking");

    PriorityWeights weights;
    scoreRequests(sc.requests, weights);
    std::vector<int> ranked = rankByPriority(sc.requests);

    std::cout << std::left
              << std::setw(6)  << "Rank"
              << std::setw(6)  << "Req"
              << std::setw(28) << "Hospital"
              << std::setw(7)  << "Group"
              << std::setw(7)  << "Units"
              << std::setw(11) << "Urgency"
              << std::setw(10) << "Critical"
              << std::setw(11) << "Deadline"
              << "Score\n";
    rule();
    for (size_t i = 0; i < ranked.size(); ++i) {
        const Request& r = sc.requests[ranked[i]];
        std::cout << std::left
                  << std::setw(6)  << (i + 1)
                  << std::setw(6)  << ("R" + std::to_string(r.id))
                  << std::setw(28) << r.hospitalName
                  << std::setw(7)  << groupName(r.group)
                  << std::setw(7)  << r.units
                  << std::setw(11) << urgencyName(r.urgency)
                  << std::setw(10) << r.criticalPatients
                  << std::setw(11) << fmtTime(r.deadline)
                  << std::fixed << std::setprecision(1) << r.priority << "\n";
    }

    // -----------------------------------------------------------------------
    // Stage 3: allocation by tiered max-flow (Edmonds-Karp)
    // -----------------------------------------------------------------------
    heading("STAGE 3  Supply allocation (Edmonds-Karp max-flow, by urgency tier)");

    // Working copy of stock; each tier consumes from it.
    std::vector<std::array<int, NUM_GROUPS>> stock(sc.banks.size());
    for (size_t b = 0; b < sc.banks.size(); ++b) stock[b] = sc.banks[b].stock;

    std::vector<Allocation> allocations;

    // One max-flow run over a chosen set of requests.
    // `exactOnly` restricts donors to the patient's own blood group; the
    // second pass then allows compatible substitutes. Running the exact pass
    // first conserves universal-donor stock (O-) for the patients who have no
    // alternative, which a single unrestricted max-flow would not do -- it
    // treats every feasible unit as interchangeable.
    auto runTier = [&](const std::vector<int>& tierReqs, bool exactOnly) -> int {
        struct BG { int bank; BloodGroup group; int node; };
        std::vector<BG> bgs;
        int nextNode = 1;
        for (size_t b = 0; b < sc.banks.size(); ++b)
            for (int g = 0; g < NUM_GROUPS; ++g)
                if (stock[b][g] > 0)
                    bgs.push_back({(int)b, (BloodGroup)g, nextNode++});

        std::vector<int> reqNode(tierReqs.size());
        for (size_t i = 0; i < tierReqs.size(); ++i) reqNode[i] = nextNode++;
        int sink = nextNode++;

        MaxFlow mf(nextNode);
        for (const BG& x : bgs) mf.addEdge(0, x.node, stock[x.bank][x.group]);

        // Remember the edge index of each (bank-group, request) pair so the
        // allocation can be read back out of the final flow.
        struct Link { int bank; BloodGroup group; int reqIdx; int edge; };
        std::vector<Link> links;

        bool any = false;
        for (size_t i = 0; i < tierReqs.size(); ++i) {
            const Request& r = sc.requests[tierReqs[i]];
            int remaining = r.units - r.allocated;
            if (remaining <= 0) continue;
            for (const BG& x : bgs) {
                // Three hard feasibility conditions, all encoded as
                // "this edge exists" rather than as a cost:
                //   1. the group must be transfusable into this patient
                //   2. on the exact pass, it must be the patient's own group
                //   3. the delivery must arrive before the deadline
                if (!compatible(x.group, r.group)) continue;
                if (exactOnly && x.group != r.group) continue;
                double t = sp[x.bank].dist[r.hospitalNode];
                if (t == INF_TIME || t > r.deadline) continue;

                int cap = std::min(stock[x.bank][x.group], remaining);
                if (cap <= 0) continue;
                links.push_back({x.bank, x.group, tierReqs[i], mf.addEdge(x.node, reqNode[i], cap)});
                any = true;
            }
            mf.addEdge(reqNode[i], sink, remaining);
        }
        if (!any) return 0;

        int pushed = mf.run(0, sink);

        for (const Link& L : links) {
            int f = mf.flowOn(L.edge);
            if (f <= 0) continue;
            stock[L.bank][L.group] -= f;
            sc.requests[L.reqIdx].allocated += f;

            Allocation a;
            a.bankIndex  = L.bank;
            a.requestId  = sc.requests[L.reqIdx].id;
            a.group      = L.group;
            a.units      = f;
            a.travelTime = sp[L.bank].dist[sc.requests[L.reqIdx].hospitalNode];
            allocations.push_back(a);

            const Request& r = sc.requests[L.reqIdx];
            std::cout << "    " << std::left << std::setw(22) << sc.banks[L.bank].name
                      << " -> R" << r.id << " " << std::setw(26) << r.hospitalName
                      << std::setw(3) << f << " units of " << std::setw(4) << groupName(L.group)
                      << (L.group == r.group ? "(exact)     " : "(substitute)")
                      << "  " << fmtTime(a.travelTime) << "\n";
        }
        return pushed;
    };

    // Tier by tier, CRITICAL first. Running the flow in urgency tiers is what
    // makes the greedy triage decision actually bind: one global max-flow
    // would maximise total units delivered, but could starve a critical
    // request in order to satisfy two routine ones.
    for (int tier = CRITICAL; tier >= LOW; --tier) {
        std::vector<int> tierReqs;
        for (int idx : ranked)
            if (sc.requests[idx].urgency == tier &&
                sc.requests[idx].allocated < sc.requests[idx].units)
                tierReqs.push_back(idx);
        if (tierReqs.empty()) continue;

        std::cout << "Tier " << std::left << std::setw(10) << urgencyName((Urgency)tier)
                  << " requests: " << tierReqs.size() << "\n";

        int exact = runTier(tierReqs, true);    // pass 1: same blood group only
        int subst = runTier(tierReqs, false);   // pass 2: compatible substitutes

        std::cout << "    subtotal: " << (exact + subst) << " units ("
                  << exact << " exact-match, " << subst << " substituted)\n";
    }

    // -----------------------------------------------------------------------
    // Stage 4: vehicle loading (0/1 knapsack DP)
    // -----------------------------------------------------------------------
    heading("STAGE 4  Vehicle loading (0/1 knapsack dynamic programming)");

    auto requestById = [&](int id) -> Request& {
        for (Request& r : sc.requests) if (r.id == id) return r;
        return sc.requests[0];   // unreachable with well-formed data
    };

    // An allocation larger than any single vehicle at that bank is split into
    // parcels, so that every item is at least loadable by some vehicle.
    std::vector<Allocation> parcels;
    for (const Allocation& a : allocations) {
        int maxCap = 0;
        for (const Vehicle& v : sc.vehicles)
            if (v.bankIndex == a.bankIndex) maxCap = std::max(maxCap, v.capacity);
        if (maxCap <= 0) { parcels.push_back(a); continue; }

        int left = a.units;
        while (left > 0) {
            Allocation p = a;
            p.units = std::min(left, maxCap);
            parcels.push_back(p);
            left -= p.units;
        }
    }

    std::vector<bool> loaded(parcels.size(), false);
    std::vector<Trip> trips;

    for (size_t v = 0; v < sc.vehicles.size(); ++v) {
        const Vehicle& veh = sc.vehicles[v];

        std::vector<int>    idxMap;
        std::vector<int>    w;
        std::vector<double> val;
        for (size_t p = 0; p < parcels.size(); ++p) {
            if (loaded[p] || parcels[p].bankIndex != veh.bankIndex) continue;
            idxMap.push_back((int)p);
            w.push_back(parcels[p].units);
            // Value = priority-weighted units. Loading this vehicle well means
            // carrying the units that matter most, not simply the most units.
            val.push_back(requestById(parcels[p].requestId).priority * parcels[p].units);
        }
        if (idxMap.empty()) continue;

        KnapsackResult kr = knapsack(w, val, veh.capacity);

        Trip t;
        t.vehicleIndex  = (int)v;
        t.priorityValue = kr.bestValue;
        t.unitsCarried  = kr.usedCapacity;
        for (int c : kr.chosen) {
            loaded[idxMap[c]] = true;
            t.load.push_back(parcels[idxMap[c]]);
        }
        trips.push_back(t);

        std::cout << std::left << std::setw(16) << veh.name
                  << "capacity " << std::setw(4) << veh.capacity
                  << "| loaded " << std::setw(4) << t.unitsCarried
                  << "| items " << t.load.size()
                  << " | priority value " << std::fixed << std::setprecision(0)
                  << t.priorityValue << "\n";
    }

    // -----------------------------------------------------------------------
    // Stage 5: dispatch plan and unmet demand
    // -----------------------------------------------------------------------
    heading("STAGE 5  Dispatch plan");

    for (const Trip& t : trips) {
        const Vehicle& veh = sc.vehicles[t.vehicleIndex];
        std::cout << "\n" << veh.name << " from " << sc.banks[veh.bankIndex].name
                  << "  (" << t.unitsCarried << "/" << veh.capacity << " units)\n";
        if (t.load.empty()) { std::cout << "    idle\n"; continue; }
        for (const Allocation& a : t.load) {
            const Request& r = requestById(a.requestId);
            std::cout << "    R" << r.id << "  " << std::left << std::setw(28) << r.hospitalName
                      << a.units << " units of " << std::setw(5) << groupName(a.group)
                      << "ETA " << fmtTime(a.travelTime) << "  (deadline "
                      << fmtTime(r.deadline) << ")\n";
            std::vector<int> path = reconstructPath(sp[veh.bankIndex],
                                                    sc.banks[veh.bankIndex].nodeId,
                                                    r.hospitalNode);
            std::cout << "          route: ";
            for (size_t i = 0; i < path.size(); ++i)
                std::cout << (i ? " -> " : "") << sc.nodes[path[i]].name;
            std::cout << "\n";
        }
    }

    int pendingUnits = 0;
    for (size_t p = 0; p < parcels.size(); ++p)
        if (!loaded[p]) pendingUnits += parcels[p].units;
    if (pendingUnits > 0)
        std::cout << "\nAllocated but awaiting a second trip: " << pendingUnits << " units\n";

    heading("SUMMARY");

    int demand = 0, served = 0;
    std::cout << std::left << std::setw(6) << "Req" << std::setw(28) << "Hospital"
              << std::setw(7) << "Group" << std::setw(12) << "Requested"
              << std::setw(12) << "Allocated" << "Status\n";
    rule();
    for (const Request& r : sc.requests) {
        demand += r.units;
        served += r.allocated;
        std::string status = (r.allocated == r.units) ? "fully met"
                           : (r.allocated == 0)       ? "UNMET"
                                                      : "partially met";
        std::cout << std::left << std::setw(6) << ("R" + std::to_string(r.id))
                  << std::setw(28) << r.hospitalName
                  << std::setw(7)  << groupName(r.group)
                  << std::setw(12) << r.units
                  << std::setw(12) << r.allocated
                  << status << "\n";
    }
    rule();
    double pct = demand ? (100.0 * served / demand) : 0.0;
    std::cout << "Total demand " << demand << " units | allocated " << served
              << " units | fulfilment " << std::fixed << std::setprecision(1)
              << pct << "%\n";

    return 0;
}
