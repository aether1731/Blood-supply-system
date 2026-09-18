#include "scenario.hpp"
#include <fstream>
#include <sstream>

namespace bf {

void addRoad(Scenario& s, int u, int v, double minutes) {
    if (static_cast<int>(s.adj.size()) < static_cast<int>(s.nodes.size()))
        s.adj.resize(s.nodes.size());
    s.adj[u].push_back({v, minutes});
    s.adj[v].push_back({u, minutes});   // roads are two-way
}

// ---------------------------------------------------------------------------
// Built-in sample: 2 blood banks, 3 hospitals, 9 junctions, 3 vehicles.
// Total O- demand (45 units) deliberately exceeds total O- supply (30 units)
// so the scarcity behaviour of the pipeline is visible in the demo.
// ---------------------------------------------------------------------------
Scenario buildSampleScenario() {
    Scenario s;
    s.title = "Metropolitan emergency: multi-hospital blood request surge";

    auto node = [&](const std::string& name, NodeType t) {
        Node n;
        n.id = static_cast<int>(s.nodes.size());
        n.name = name;
        n.type = t;
        s.nodes.push_back(n);
        return n.id;
    };

    int B1 = node("Central Blood Bank",        BANK_NODE);      // 0
    int B2 = node("Northside Blood Bank",      BANK_NODE);      // 1
    int H1 = node("City General Hospital",     HOSPITAL_NODE);  // 2
    int H2 = node("Riverside Medical College", HOSPITAL_NODE);  // 3
    int H3 = node("Metro Trauma Center",       HOSPITAL_NODE);  // 4
    int J1 = node("Junction J1", JUNCTION_NODE);                // 5
    int J2 = node("Junction J2", JUNCTION_NODE);                // 6
    int J3 = node("Junction J3", JUNCTION_NODE);                // 7
    int J4 = node("Junction J4", JUNCTION_NODE);                // 8
    int J5 = node("Junction J5", JUNCTION_NODE);                // 9
    int J6 = node("Junction J6", JUNCTION_NODE);                // 10
    int J7 = node("Junction J7", JUNCTION_NODE);                // 11
    int J8 = node("Junction J8", JUNCTION_NODE);                // 12
    int J9 = node("Junction J9", JUNCTION_NODE);                // 13

    s.adj.resize(s.nodes.size());

    addRoad(s, B1, J1,  4);
    addRoad(s, B1, J2,  6);
    addRoad(s, B1, J9, 11);
    addRoad(s, J1, J3,  5);
    addRoad(s, J1, H1, 12);
    addRoad(s, J2, J3,  3);
    addRoad(s, J2, J6,  8);
    addRoad(s, J3, H1,  4);
    addRoad(s, J3, J5,  7);
    addRoad(s, B2, J3, 12);
    addRoad(s, B2, J4,  5);
    addRoad(s, J4, J5,  4);
    addRoad(s, J4, J8,  9);
    addRoad(s, J5, H2,  6);
    addRoad(s, J5, J7,  5);
    addRoad(s, J6, H3,  9);
    addRoad(s, J6, J7,  4);
    addRoad(s, J7, H3,  6);
    addRoad(s, J8, H1, 10);
    addRoad(s, J9, H2, 14);

    BloodBank b1;
    b1.nodeId = B1;
    b1.name   = "Central Blood Bank";
    b1.stock[O_NEG] = 18; b1.stock[O_POS] = 40; b1.stock[A_POS] = 25;
    b1.stock[B_POS] = 10; b1.stock[AB_POS] = 5;
    s.banks.push_back(b1);

    BloodBank b2;
    b2.nodeId = B2;
    b2.name   = "Northside Blood Bank";
    b2.stock[O_NEG] = 12; b2.stock[A_POS] = 30; b2.stock[A_NEG] = 8;
    b2.stock[B_POS] = 14;
    s.banks.push_back(b2);

    auto req = [&](int hospital, const std::string& hname, BloodGroup g,
                   int units, Urgency u, int patients, double deadline) {
        Request r;
        r.id               = static_cast<int>(s.requests.size()) + 1;
        r.hospitalNode     = hospital;
        r.hospitalName     = hname;
        r.group            = g;
        r.units            = units;
        r.urgency          = u;
        r.criticalPatients = patients;
        r.deadline         = deadline;
        s.requests.push_back(r);
    };

    req(H1, "City General Hospital",     O_NEG, 20, CRITICAL, 9, 30);
    req(H2, "Riverside Medical College", A_POS, 30, MEDIUM,   2, 90);
    req(H3, "Metro Trauma Center",       O_NEG, 15, HIGH,     6, 45);
    req(H1, "City General Hospital",     B_POS, 12, HIGH,     3, 60);
    req(H2, "Riverside Medical College", O_NEG, 10, CRITICAL, 7, 40);

    s.vehicles.push_back({0, "V1 (Central)",   25});
    s.vehicles.push_back({0, "V2 (Central)",   20});
    s.vehicles.push_back({1, "V3 (Northside)", 30});

    return s;
}

// ---------------------------------------------------------------------------
// File parser
// ---------------------------------------------------------------------------
static bool nextToken(std::istream& in, std::string& tok) {
    while (in >> tok) {
        if (!tok.empty() && tok[0] == '#') {          // comment to end of line
            std::string rest;
            std::getline(in, rest);
            continue;
        }
        return true;
    }
    return false;
}

bool loadScenario(const std::string& path, Scenario& out, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "cannot open file: " + path; return false; }

    Scenario s;
    std::string tok;

    auto need = [&](std::string& t) -> bool { return nextToken(in, t); };

    while (need(tok)) {
        if (tok == "TITLE") {
            std::string rest;
            std::getline(in, rest);
            size_t a = rest.find_first_not_of(" \t");
            s.title = (a == std::string::npos) ? "Untitled scenario" : rest.substr(a);
        } else if (tok == "NODES") {
            int n;
            if (!(in >> n)) { error = "NODES: missing count"; return false; }
            s.nodes.resize(n);
            for (int i = 0; i < n; ++i) {
                int id; std::string name, type;
                if (!(in >> id >> name >> type)) { error = "NODES: bad row"; return false; }
                if (id < 0 || id >= n) { error = "NODES: id out of range"; return false; }
                for (char& c : name) if (c == '_') c = ' ';
                s.nodes[id].id   = id;
                s.nodes[id].name = name;
                s.nodes[id].type = (type == "BANK")     ? BANK_NODE
                                 : (type == "HOSPITAL") ? HOSPITAL_NODE
                                                        : JUNCTION_NODE;
            }
            s.adj.assign(n, {});
        } else if (tok == "EDGES") {
            int m;
            if (!(in >> m)) { error = "EDGES: missing count"; return false; }
            for (int i = 0; i < m; ++i) {
                int u, v; double w;
                if (!(in >> u >> v >> w)) { error = "EDGES: bad row"; return false; }
                if (u < 0 || v < 0 || u >= (int)s.nodes.size() || v >= (int)s.nodes.size()) {
                    error = "EDGES: node id out of range"; return false;
                }
                if (w < 0) { error = "EDGES: negative travel time"; return false; }
                addRoad(s, u, v, w);
            }
        } else if (tok == "BANKS") {
            int k;
            if (!(in >> k)) { error = "BANKS: missing count"; return false; }
            for (int i = 0; i < k; ++i) {
                int nodeId, groups;
                if (!(in >> nodeId >> groups)) { error = "BANKS: bad header"; return false; }
                BloodBank b;
                b.nodeId = nodeId;
                b.name   = (nodeId < (int)s.nodes.size()) ? s.nodes[nodeId].name : "Bank";
                for (int j = 0; j < groups; ++j) {
                    std::string g; int qty;
                    if (!(in >> g >> qty)) { error = "BANKS: bad stock row"; return false; }
                    BloodGroup bg;
                    if (!parseGroup(g, bg)) { error = "BANKS: unknown blood group " + g; return false; }
                    b.stock[bg] += qty;
                }
                s.banks.push_back(b);
            }
        } else if (tok == "REQUESTS") {
            int n;
            if (!(in >> n)) { error = "REQUESTS: missing count"; return false; }
            for (int i = 0; i < n; ++i) {
                int hosp, units, patients; double deadline;
                std::string g, u;
                if (!(in >> hosp >> g >> units >> u >> patients >> deadline)) {
                    error = "REQUESTS: bad row"; return false;
                }
                Request r;
                r.id           = (int)s.requests.size() + 1;
                r.hospitalNode = hosp;
                r.hospitalName = (hosp < (int)s.nodes.size()) ? s.nodes[hosp].name : "Hospital";
                if (!parseGroup(g, r.group))     { error = "REQUESTS: unknown blood group " + g; return false; }
                if (!parseUrgency(u, r.urgency)) { error = "REQUESTS: unknown urgency " + u; return false; }
                r.units            = units;
                r.criticalPatients = patients;
                r.deadline         = deadline;
                s.requests.push_back(r);
            }
        } else if (tok == "VEHICLES") {
            int n;
            if (!(in >> n)) { error = "VEHICLES: missing count"; return false; }
            for (int i = 0; i < n; ++i) {
                int bankIdx, cap; std::string name;
                if (!(in >> bankIdx >> cap >> name)) { error = "VEHICLES: bad row"; return false; }
                for (char& c : name) if (c == '_') c = ' ';
                s.vehicles.push_back({bankIdx, name, cap});
            }
        } else {
            error = "unexpected token: " + tok;
            return false;
        }
    }

    if (s.nodes.empty())    { error = "scenario has no nodes"; return false; }
    if (s.banks.empty())    { error = "scenario has no blood banks"; return false; }
    if (s.requests.empty()) { error = "scenario has no requests"; return false; }

    out = s;
    return true;
}

} // namespace bf
