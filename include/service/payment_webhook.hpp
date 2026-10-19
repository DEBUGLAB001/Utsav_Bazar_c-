#pragma once
#include "model/domain.hpp"
namespace utsav { void apply_payment_webhook(Order& order, bool payment_succeeded); const char* status_name(OrderStatus status); }
