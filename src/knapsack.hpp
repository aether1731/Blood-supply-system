// knapsack.hpp -- Stage 4 of the pipeline: loading delivery vehicles.
#ifndef KNAPSACK_HPP
#define KNAPSACK_HPP

#include <vector>

namespace bf {

// 0/1 knapsack solved by bottom-up dynamic programming.
//
// Items are candidate deliveries: weight = units of blood, value = the
// priority value of getting that delivery onto this trip. Capacity is the
// vehicle's carrying limit.
//
// Recurrence:
//   dp[i][c] = max( dp[i-1][c],                                  // skip item i
//                   dp[i-1][c - weight[i]] + value[i] )          // take item i
//
// Complexity: O(n * capacity) time, O(n * capacity) memory for the choice
// table needed to reconstruct which items were taken.
struct KnapsackResult {
    double            bestValue = 0.0;
    int               usedCapacity = 0;
    std::vector<int>  chosen;   // indices of the selected items
};

KnapsackResult knapsack(const std::vector<int>& weights,
                        const std::vector<double>& values,
                        int capacity);

} // namespace bf

#endif // KNAPSACK_HPP
