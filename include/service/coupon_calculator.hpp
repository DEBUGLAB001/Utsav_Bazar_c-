#pragma once
#include "model/domain.hpp"
namespace utsav { int coupon_discount_cents(const std::vector<CouponOrder>& orders, int percent, int cap_per_order_cents); }
