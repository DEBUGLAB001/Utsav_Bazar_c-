#pragma once
#include <atomic>
namespace utsav {
class StockLedger {
 public: explicit StockLedger(int stock); bool reserve(int quantity); int available() const;
 private: int capacity_; std::atomic<int> reserved_{0};
};
}
