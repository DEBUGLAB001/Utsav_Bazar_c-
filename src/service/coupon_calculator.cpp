#include "service/coupon_calculator.hpp"
#include <algorithm>
namespace utsav {
int coupon_discount_cents(const std::vector<CouponOrder>& orders, int percent, int cap_per_order_cents) {
    int total = 0;
    for (const auto& order : orders) total += order.subtotal_cents * percent / 100;
    // BUG 6: the cap is applied once to the entire batch instead of separately to each order.
    return std::min(total, cap_per_order_cents);
}
}
