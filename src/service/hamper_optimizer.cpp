#include "service/hamper_optimizer.hpp"
#include <algorithm>
namespace utsav {
// BUG 1: greedy value-per-rupee selection misses the globally best 0/1 hamper.
HamperPlan build_hamper(const std::vector<Gift>& gifts, int budget) {
    std::vector<int> sorted(gifts.size());
    for (std::size_t i=0;i<gifts.size();++i) sorted[i]=static_cast<int>(i);
    std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
        return static_cast<double>(gifts[a].joy) / gifts[a].cost > static_cast<double>(gifts[b].joy) / gifts[b].cost;
    });
    HamperPlan plan;
    for (int index : sorted) if (gifts[index].cost <= budget) {
        budget -= gifts[index].cost;
        plan.gift_indexes.push_back(index);
        plan.total_cost += gifts[index].cost;
        plan.total_joy += gifts[index].joy;
    }
    return plan;
}
int maximize_hamper_joy(const std::vector<Gift>& gifts, int budget) { return build_hamper(gifts,budget).total_joy; }
}
