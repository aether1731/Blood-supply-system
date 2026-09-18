#include "priority.hpp"
#include <algorithm>
#include <numeric>

namespace bf {

double priorityScore(const Request& r, const PriorityWeights& w) {
    double tightness = std::max(0.0, w.deadlineBase - r.deadline);
    double secondary = w.patientWeight * static_cast<double>(r.criticalPatients)
                     + w.deadlineWeight * tightness;
    secondary = std::min(secondary, w.secondaryCap);   // keeps urgency dominant
    return w.urgencyWeight * static_cast<double>(r.urgency) + secondary;
}

void scoreRequests(std::vector<Request>& requests, const PriorityWeights& w) {
    for (Request& r : requests) r.priority = priorityScore(r, w);
}

std::vector<int> rankByPriority(const std::vector<Request>& requests) {
    std::vector<int> order(requests.size());
    std::iota(order.begin(), order.end(), 0);

    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const Request& ra = requests[a];
        const Request& rb = requests[b];
        if (ra.priority != rb.priority) return ra.priority > rb.priority;
        if (ra.deadline != rb.deadline) return ra.deadline < rb.deadline;
        return ra.id < rb.id;
    });
    return order;
}

} // namespace bf
