#include "service/light_trail.hpp"
#include <algorithm>
#include <queue>
namespace utsav {
// BUG 3: breadth-first search minimizes hops, but this network asks for minimum total distance.
TrailPlan find_light_trail(const std::vector<std::vector<std::pair<int,int>>>& graph, int start, int goal) {
    TrailPlan plan;
    if(start<0||goal<0||start>=static_cast<int>(graph.size())||goal>=static_cast<int>(graph.size())) return plan;
    std::vector<int> distance(graph.size(), -1), parent(graph.size(),-1);
    std::queue<int> pending; distance[start] = 0; pending.push(start);
    while (!pending.empty()) {
        int node = pending.front(); pending.pop();
        if (node == goal) {
            plan.distance=distance[node];
            for(int step=goal;step!=-1;step=parent[step]) plan.path.push_back(step);
            std::reverse(plan.path.begin(),plan.path.end());
            return plan;
        }
        for (const auto& edge : graph[node]) if (distance[edge.first] == -1) {
            distance[edge.first] = distance[node] + edge.second; parent[edge.first]=node; pending.push(edge.first);
        }
    }
    return plan;
}
int shortest_light_trail(const std::vector<std::vector<std::pair<int,int>>>& graph, int start, int goal) { return find_light_trail(graph,start,goal).distance; }
}
