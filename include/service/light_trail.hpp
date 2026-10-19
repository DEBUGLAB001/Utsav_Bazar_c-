#pragma once
#include "model/domain.hpp"
#include <utility>
#include <vector>
namespace utsav { int shortest_light_trail(const std::vector<std::vector<std::pair<int,int>>>& graph, int start, int goal); }
namespace utsav { TrailPlan find_light_trail(const std::vector<std::vector<std::pair<int,int>>>& graph, int start, int goal); }
