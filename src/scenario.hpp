// scenario.hpp -- Problem-instance input: built-in sample or a text file.
#ifndef SCENARIO_HPP
#define SCENARIO_HPP

#include "model.hpp"
#include <string>

namespace bf {

// The worked example used in the report and the demo video.
Scenario buildSampleScenario();

// Parses a scenario file. Returns false and fills `error` on a malformed file.
// Format is documented in data/sample_scenario.txt.
bool loadScenario(const std::string& path, Scenario& out, std::string& error);

// Builds the adjacency list from an edge list. Roads are bidirectional.
void addRoad(Scenario& s, int u, int v, double minutes);

} // namespace bf

#endif // SCENARIO_HPP
