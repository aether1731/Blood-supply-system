#include "model.hpp"
#include <algorithm>
#include <cctype>

namespace bf {

static const char* kGroupNames[NUM_GROUPS] = {
    "O-", "O+", "A-", "A+", "B-", "B+", "AB-", "AB+"
};

const char* groupName(BloodGroup g) { return kGroupNames[g]; }

bool parseGroup(const std::string& s, BloodGroup& out) {
    for (int i = 0; i < NUM_GROUPS; ++i) {
        if (s == kGroupNames[i]) { out = static_cast<BloodGroup>(i); return true; }
    }
    return false;
}

// Row = donor, column = recipient. A 1 means the transfusion is permitted.
//               O-  O+  A-  A+  B-  B+ AB- AB+
static const bool kCompat[NUM_GROUPS][NUM_GROUPS] = {
/* O-  */    { 1,  1,  1,  1,  1,  1,  1,  1 },
/* O+  */    { 0,  1,  0,  1,  0,  1,  0,  1 },
/* A-  */    { 0,  0,  1,  1,  0,  0,  1,  1 },
/* A+  */    { 0,  0,  0,  1,  0,  0,  0,  1 },
/* B-  */    { 0,  0,  0,  0,  1,  1,  1,  1 },
/* B+  */    { 0,  0,  0,  0,  0,  1,  0,  1 },
/* AB- */    { 0,  0,  0,  0,  0,  0,  1,  1 },
/* AB+ */    { 0,  0,  0,  0,  0,  0,  0,  1 }
};

bool compatible(BloodGroup donor, BloodGroup recipient) {
    return kCompat[donor][recipient];
}

static const char* kUrgencyNames[NUM_URGENCY] = { "LOW", "MEDIUM", "HIGH", "CRITICAL" };

const char* urgencyName(Urgency u) { return kUrgencyNames[u]; }

bool parseUrgency(const std::string& s, Urgency& out) {
    std::string up = s;
    std::transform(up.begin(), up.end(), up.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    for (int i = 0; i < NUM_URGENCY; ++i) {
        if (up == kUrgencyNames[i]) { out = static_cast<Urgency>(i); return true; }
    }
    return false;
}

} // namespace bf
