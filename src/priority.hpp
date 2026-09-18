// priority.hpp -- Stage 2 of the pipeline: greedy triage of hospital requests.
#ifndef PRIORITY_HPP
#define PRIORITY_HPP

#include "model.hpp"
#include <vector>

namespace bf {

// Weights of the greedy priority score. Exposed as a struct so the triage
// policy can be re-tuned by a hospital authority without touching the
// algorithm itself.
struct PriorityWeights {
    double urgencyWeight  = 10000.0; // per urgency tier (dominant term)
    double patientWeight  =     5.0; // per critical patient waiting
    double deadlineBase   =   120.0; // slack budget, minutes
    double deadlineWeight =     1.0; // per minute of deadline tightness
    double secondaryCap   =   999.0; // ceiling on the non-urgency terms
};

// score(r) = w_u * urgency + min(secondaryCap, w_p * criticalPatients
//                                            + w_d * max(0, deadlineBase - deadline))
//
// The greedy choice is "always serve the highest-scoring unserved request
// next". The score is deliberately lexicographic: capping the secondary terms
// below w_u guarantees that no quantity of low-urgency patients, and no
// deadline however tight, can outrank a CRITICAL request. The medical policy
// being encoded is triage, not throughput. The secondary terms only order
// requests that already sit in the same urgency tier.
double priorityScore(const Request& r, const PriorityWeights& w);

// Computes and stores the score for every request.
void scoreRequests(std::vector<Request>& requests, const PriorityWeights& w);

// Returns request indices sorted by descending priority score.
// Ties are broken by the tighter deadline, then by the smaller request id, so
// the ordering is deterministic and reproducible between runs.
std::vector<int> rankByPriority(const std::vector<Request>& requests);

} // namespace bf

#endif // PRIORITY_HPP
