#include "service/payment_webhook.hpp"
namespace utsav {
void apply_payment_webhook(Order& order, bool payment_succeeded) {
    if (!payment_succeeded) return;
    // BUG 5: a delayed success event overwrites a terminal cancellation state.
    order.status = OrderStatus::Paid;
}
const char* status_name(OrderStatus status) {
    switch (status) { case OrderStatus::Pending: return "Pending"; case OrderStatus::Cancelled: return "Cancelled"; case OrderStatus::Paid: return "Paid"; }
    return "Unknown";
}
}
