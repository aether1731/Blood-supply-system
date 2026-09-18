# Emergency Blood Supply Distribution and Hospital Allocation System

CSE 4403 (Algorithms) — Assignment 2 (Quiz 4)

A decision-support tool that decides **which blood bank serves which hospital, how many
units to send, which route to take, and how to load the delivery vehicles** during a
mass-casualty or disaster event.

The program is a console application in C++17 with no external dependencies.

---

## Build and run

```bash
make            # builds ./bloodflow
make run        # runs the built-in sample scenario
make sample     # runs data/sample_scenario.txt (identical data, read from file)
make test       # builds and runs the algorithm verification suite
make clean
```

Run against your own scenario:

```bash
./bloodflow data/sample_scenario.txt
```

With no argument the program falls back to the built-in sample, so it always produces
output for a demo.

---

## What the program does

Five stages, each the output of the one before it:

| Stage | Algorithm | Question it answers |
|---|---|---|
| 1 | Dijkstra's shortest path | How fast can each bank reach each hospital, and by what route? |
| 2 | Greedy triage scoring | Which requests get considered first? |
| 3 | Edmonds–Karp max-flow | How many units can feasibly go from which bank to which request? |
| 4 | 0/1 knapsack (DP) | What does each vehicle carry on this trip? |
| 5 | Dispatch report | Final plan, routes, and unmet demand. |

Two design decisions are worth highlighting, because a naive implementation gets them
wrong:

- **The max-flow runs in urgency tiers, not once globally.** A single unrestricted
  max-flow maximises total units delivered and will happily starve one critical request
  to satisfy two routine ones. Running CRITICAL first, then HIGH, and so on, makes the
  triage ranking actually bind.
- **Within each tier, exact blood-group matches are allocated before compatible
  substitutes.** Otherwise the flow treats every feasible unit as interchangeable and
  will spend scarce O-negative stock on a patient who could have received A-positive.

---

## Source layout

```
src/model.hpp / .cpp     Data model, blood-group compatibility matrix, urgency tiers
src/graph.hpp / .cpp     Dijkstra with a binary-heap priority queue, path reconstruction
src/priority.hpp / .cpp  Greedy priority score and ranking
src/maxflow.hpp / .cpp   Edmonds–Karp maximum flow with residual edges
src/knapsack.hpp / .cpp  0/1 knapsack DP with traceback
src/scenario.hpp / .cpp  Built-in sample scenario and the text-file parser
src/main.cpp             Pipeline driver and report output
tests/                   Verification suite against hand-computed answers
data/                    Sample scenario file, with the input format documented inline
```

---

## Scenario file format

Documented in full at the top of `data/sample_scenario.txt`. In brief:

```
TITLE     <free text>
NODES     <n>   then: <id> <name_with_underscores> <BANK|HOSPITAL|JUNCTION>
EDGES     <m>   then: <u> <v> <travel_minutes>
BANKS     <k>   then per bank: <nodeId> <numGroups>, then <group> <units> rows
REQUESTS  <n>   then: <hospitalNodeId> <group> <units> <urgency> <criticalPatients> <deadlineMinutes>
VEHICLES  <n>   then: <bankIndex> <capacity> <name_with_underscores>
```

Blood groups: `O- O+ A- A+ B- B+ AB- AB+`  ·  Urgency: `LOW MEDIUM HIGH CRITICAL`

---

## Complexity

| Component | Time | Notes |
|---|---|---|
| Dijkstra (per bank) | O((V + E) log V) | Run once per blood bank, so O(B · (V+E) log V) overall |
| Greedy scoring + sort | O(R log R) | R = number of requests |
| Edmonds–Karp (per tier/pass) | O(V' · E'²) | V', E' are the flow network's size, not the road network's |
| Knapsack (per vehicle) | O(n · C) | n = candidate parcels, C = vehicle capacity |

---

## Known limitations

- Travel times are static. A live system would re-run stage 1 as roads close.
- Each vehicle makes one trip; leftover allocations are reported as awaiting a second trip
  rather than scheduled into one.
- The knapsack is 0/1 over parcels, so a delivery is split only when it exceeds the
  largest vehicle at its bank.
- Blood shelf life, cold-chain limits, and crossmatching beyond ABO/Rh are not modelled.
