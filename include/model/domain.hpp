#pragma once
#include <string>
#include <vector>
namespace utsav {
struct Gift { std::string name; int cost; int joy; };
struct Event { std::string name; int start; int finish; };
struct HamperPlan { std::vector<int> gift_indexes; int total_cost = 0; int total_joy = 0; };
struct TrailPlan { int distance = -1; std::vector<int> path; };
struct CouponOrder { int subtotal_cents; };
enum class OrderStatus { Pending, Cancelled, Paid };
struct Order { std::string id; OrderStatus status = OrderStatus::Pending; };
}
