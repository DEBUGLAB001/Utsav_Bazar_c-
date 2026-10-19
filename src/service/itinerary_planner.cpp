#include "service/itinerary_planner.hpp"
#include <algorithm>
namespace utsav {
// BUG 2: interval scheduling must prioritize earliest finish time, not earliest start.
std::vector<int> plan_itinerary(const std::vector<Event>& events) {
    std::vector<int> sorted(events.size());
    for (std::size_t i=0;i<events.size();++i) sorted[i]=static_cast<int>(i);
    std::sort(sorted.begin(), sorted.end(), [&](int a, int b) { return events[a].start < events[b].start; });
    std::vector<int> selected;
    int last_finish = -1;
    for (int index : sorted) if (events[index].start >= last_finish) { selected.push_back(index); last_finish = events[index].finish; }
    return selected;
}
int select_event_count(const std::vector<Event>& events) { return static_cast<int>(plan_itinerary(events).size()); }
}
