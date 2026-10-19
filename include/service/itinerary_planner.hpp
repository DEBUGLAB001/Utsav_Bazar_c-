#pragma once
#include "model/domain.hpp"
namespace utsav { int select_event_count(const std::vector<Event>& events); }
namespace utsav { std::vector<int> plan_itinerary(const std::vector<Event>& events); }
