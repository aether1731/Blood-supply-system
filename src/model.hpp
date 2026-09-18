// model.hpp -- Core data structures for the Emergency Blood Supply Distribution system.
// CSE 4403 (Algorithms) -- Assignment 2
#ifndef MODEL_HPP
#define MODEL_HPP

#include <string>
#include <vector>
#include <array>

namespace bf {

// ---------------------------------------------------------------------------
// Blood groups
// ---------------------------------------------------------------------------
// Eight ABO/Rh red-cell groups. Kept as a fixed enum so the compatibility
// matrix below is a constant-time lookup instead of a string comparison.
enum BloodGroup {
    O_NEG = 0, O_POS, A_NEG, A_POS, B_NEG, B_POS, AB_NEG, AB_POS,
    NUM_GROUPS
};

const char* groupName(BloodGroup g);
bool parseGroup(const std::string& s, BloodGroup& out);

// Red-cell compatibility: can a unit of `donor` be transfused into a patient
// whose group is `recipient`? O- is the universal donor, AB+ the universal
// recipient. This is a hard medical constraint, so it is enforced as edge
// existence in the flow network rather than as a cost.
bool compatible(BloodGroup donor, BloodGroup recipient);

// ---------------------------------------------------------------------------
// Urgency tiers
// ---------------------------------------------------------------------------
// Ordered from least to most urgent so that a plain integer comparison gives
// the correct ranking. Used both by the greedy scorer and by the tiered
// max-flow stage.
enum Urgency { LOW = 0, MEDIUM = 1, HIGH = 2, CRITICAL = 3, NUM_URGENCY };

const char* urgencyName(Urgency u);
bool parseUrgency(const std::string& s, Urgency& out);

// ---------------------------------------------------------------------------
// Road network
// ---------------------------------------------------------------------------
enum NodeType { BANK_NODE, HOSPITAL_NODE, JUNCTION_NODE };

struct Node {
    int         id = 0;
    std::string name;
    NodeType    type = JUNCTION_NODE;
};

struct Road {
    int    to     = 0;
    double weight = 0.0;   // travel time in minutes
};

// ---------------------------------------------------------------------------
// Supply side
// ---------------------------------------------------------------------------
struct BloodBank {
    int         nodeId = 0;
    std::string name;
    // stock[g] = units of blood group g currently held.
    std::array<int, NUM_GROUPS> stock{};
};

struct Vehicle {
    int         bankIndex = 0;   // index into Scenario::banks
    std::string name;
    int         capacity = 0;    // units it can carry in one trip
};

// ---------------------------------------------------------------------------
// Demand side
// ---------------------------------------------------------------------------
struct Request {
    int         id = 0;
    int         hospitalNode = 0;
    std::string hospitalName;
    BloodGroup  group = O_NEG;
    int         units = 0;           // units requested
    Urgency     urgency = MEDIUM;
    int         criticalPatients = 0;
    double      deadline = 0.0;      // max acceptable delivery time, minutes

    // Filled in by later stages of the pipeline.
    double      priority = 0.0;      // greedy score
    int         allocated = 0;       // units granted by max-flow
};

// ---------------------------------------------------------------------------
// Whole problem instance
// ---------------------------------------------------------------------------
struct Scenario {
    std::string                     title;
    std::vector<Node>               nodes;
    std::vector<std::vector<Road>>  adj;      // adjacency list, indexed by node id
    std::vector<BloodBank>          banks;
    std::vector<Request>            requests;
    std::vector<Vehicle>            vehicles;
};

// A single "bank -> request" allocation decided by the max-flow stage.
struct Allocation {
    int        bankIndex  = 0;
    int        requestId  = 0;
    BloodGroup group      = O_NEG;   // the group actually shipped, which may
                                     // be a compatible substitute, not an
                                     // exact match for the request
    int        units      = 0;
    double     travelTime = 0.0;     // minutes, from the Dijkstra stage
};

// One vehicle trip produced by the knapsack stage.
struct Trip {
    int                     vehicleIndex = 0;
    std::vector<Allocation> load;
    int                     unitsCarried = 0;
    double                  priorityValue = 0.0;
};

} // namespace bf

#endif // MODEL_HPP
