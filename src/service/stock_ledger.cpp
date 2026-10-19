#include "service/stock_ledger.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <chrono>
#include <thread>
#endif
namespace utsav {
StockLedger::StockLedger(int stock) : capacity_(stock) {}
bool StockLedger::reserve(int quantity) {
    if (quantity <= 0) return false;
    const int current = reserved_.load();
    if (current + quantity > capacity_) return false;
    // BUG 4: check and update are separate, so concurrent requests can both claim the last item.
#ifdef _WIN32
    Sleep(8);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
#endif
    reserved_.store(current + quantity);
    return true;
}
int StockLedger::available() const { return capacity_ - reserved_.load(); }
}
