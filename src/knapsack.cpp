#include "knapsack.hpp"
#include <algorithm>

namespace bf {

KnapsackResult knapsack(const std::vector<int>& weights,
                        const std::vector<double>& values,
                        int capacity) {
    KnapsackResult res;
    const int n = static_cast<int>(weights.size());
    if (n == 0 || capacity <= 0) return res;

    // dp[i][c]: best value using the first i items within capacity c.
    std::vector<std::vector<double>> dp(n + 1, std::vector<double>(capacity + 1, 0.0));

    for (int i = 1; i <= n; ++i) {
        int    wi = weights[i - 1];
        double vi = values[i - 1];
        for (int c = 0; c <= capacity; ++c) {
            dp[i][c] = dp[i - 1][c];                       // do not take item i
            if (wi <= c) {
                double take = dp[i - 1][c - wi] + vi;      // take item i
                if (take > dp[i][c]) dp[i][c] = take;
            }
        }
    }

    // Traceback: walk the table backwards to recover the chosen items.
    int c = capacity;
    for (int i = n; i >= 1; --i) {
        if (dp[i][c] != dp[i - 1][c]) {      // item i was taken
            res.chosen.push_back(i - 1);
            c -= weights[i - 1];
        }
    }
    std::reverse(res.chosen.begin(), res.chosen.end());

    res.bestValue    = dp[n][capacity];
    res.usedCapacity = capacity - c;
    return res;
}

} // namespace bf
