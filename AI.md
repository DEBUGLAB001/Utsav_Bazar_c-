# Utsav Bazaar

## Description

Utsav Bazaar is a C++17 web application and backend algorithm service designed for guests and organizers of an artisan Diwali festival mela. The application entry point is `src/main.cpp`, which initializes an embedded multi-threaded HTTP server using `httplib.h` and deserializes JSON requests via `json.hpp` while serving a static single-page client dashboard from `public/`. Core business logic and algorithmic modules reside in `src/service/` (with headers in `include/service/`), providing 0/1 knapsack gift hamper optimization, interval-scheduled event itineraries, shortest-path illuminated trail routing, thread-safe inventory reservations, asynchronous payment webhook processing, and promotional coupon calculations. Domain entities (`Gift`, `Event`, `Order`, `HamperPlan`, `TrailPlan`) are defined in `include/model/domain.hpp`, and application state is managed in-memory in `src/main.cpp` using `std::mutex` and atomic ledger primitives.

## Repository Structure

```text
.
├── CMakeLists.txt              # CMake build configuration for services and server
├── challenge.json              # Challenge runtime and execution metadata
├── start.sh                    # Rebuild and run script
├── include/
│   ├── httplib.h               # Embedded HTTP server and client library
│   ├── json.hpp                # Nlohmann JSON parser
│   ├── model/
│   │   └── domain.hpp          # Domain data models (Gift, Event, Order, etc.)
│   └── service/
│       ├── coupon_calculator.hpp
│       ├── hamper_optimizer.hpp
│       ├── itinerary_planner.hpp
│       ├── light_trail.hpp
│       ├── payment_webhook.hpp
│       └── stock_ledger.hpp
├── src/
│   ├── main.cpp                # HTTP server entry point and API route handlers
│   └── service/
│       ├── coupon_calculator.cpp
│       ├── hamper_optimizer.cpp
│       ├── itinerary_planner.cpp
│       ├── light_trail.cpp
│       ├── payment_webhook.cpp
│       └── stock_ledger.cpp
└── public/
    ├── index.html              # Frontend dashboard markup
    ├── css/
    │   └── style.css           # Styling for dashboard
    └── js/
        └── app.js              # Client-side API fetch controller
```

## Bugs and Bug Locations

1. Suboptimal Gift Selection Under Budget Constraints
- Bug location: `src/service/hamper_optimizer.cpp`, within `utsav::build_hamper` (lines 5–19) and `utsav::maximize_hamper_joy` (line 20).
- How to observe it: Send an HTTP POST request to `/api/hamper` with body `{"budget": 1800}` or run test case `gift_hamper_budget_optimization`.
- Failure: The selection algorithm uses a greedy value-to-cost ratio heuristic (`joy / cost`) instead of 0/1 knapsack dynamic programming. For an ₹1,800 budget with items Kesar (cost 700, joy 95), Diya (cost 500, joy 80), Scarf (cost 1200, joy 160), and Tea (cost 900, joy 130), greedy packing picks Diya and Tea (total cost 1400, joy 210), failing to find the superior combination of Diya and Scarf (total cost 1700, joy 240).
- Expected: Implement 0/1 knapsack dynamic programming to maximize total joy subject to budget constraints, returning Diya and Scarf with 240 total joy for an ₹1,800 budget.

2. Incorrect Itinerary Scheduling Prioritizing Start Times
- Bug location: `src/service/itinerary_planner.cpp`, within `utsav::plan_itinerary` (lines 5–13) and `utsav::select_event_count` (line 14).
- How to observe it: Send an HTTP POST request to `/api/itinerary` or run test case `festival_itinerary_selection`.
- Failure: Line 8 sorts events by start time instead of finish time. When an opening ceremony starts early but runs for many hours, it is chosen first and overlaps with subsequent shorter activities, resulting in only 1 event scheduled instead of 3.
- Expected: Sort events by earliest finish time (`events[a].finish < events[b].finish`) in the greedy interval scheduling routine to maximize non-overlapping events, scheduling 3 distinct events.

3. Suboptimal Walking Route Calculation on Weighted Trails
- Bug location: `src/service/light_trail.cpp`, within `utsav::find_light_trail` (lines 6–24) and `utsav::shortest_light_trail` (line 25).
- How to observe it: Send an HTTP POST request to `/api/light-trail` or run test case `weighted_light_trail_route`.
- Failure: The pathfinder implements an unweighted Breadth-First Search (BFS) using a FIFO queue, minimizing hop count rather than accumulated distance. On a weighted trail network, it selects a direct 1-hop path of 10 minutes instead of a 2-hop path of 2 minutes ($0 \to 2 \to 1$).
- Expected: Implement Dijkstra's algorithm with a priority queue to relax edge distances and find the true minimum-weight path with distance 2.

4. Race Condition in Concurrent Stock Reservations
- Bug location: `src/service/stock_ledger.cpp`, within `utsav::StockLedger::reserve` (lines 10–22).
- How to observe it: Send concurrent HTTP POST requests to `/api/reservations` with `{"quantity": 1}` when capacity is 1, or run test case `limited_stock_concurrent_reservations`.
- Failure: Remaining capacity inspection (`reserved_.load()`) and state updating (`reserved_.store()`) are separated by a thread sleep delay without synchronization. Two concurrent threads both read available capacity and store the increment, overselling the single remaining artisan diya.
- Expected: Validate and update inventory atomically (using compare-and-swap or mutex locking) so exactly one concurrent request succeeds when 1 item remains and subsequent requests return false.

5. Delayed Payment Webhook Overwrites Order Cancellation
- Bug location: `src/service/payment_webhook.cpp`, within `utsav::apply_payment_webhook` (lines 3–7).
- How to observe it: Cancel an active order via `/api/orders/cancel` and trigger `/api/orders/payment-update`, or run test case `late_payment_webhook_after_cancellation`.
- Failure: Line 6 unconditionally overwrites `order.status` to `OrderStatus::Paid` without checking current order state, overwriting the terminal `OrderStatus::Cancelled` status when a delayed payment gateway webhook arrives.
- Expected: Preserve terminal order states by ignoring payment success webhooks if `order.status` is `OrderStatus::Cancelled`, transitioning to `OrderStatus::Paid` only from `OrderStatus::Pending`.

6. Global Coupon Discount Cap Applied Across Entire Batch
- Bug location: `src/service/coupon_calculator.cpp`, within `utsav::coupon_discount_cents` (lines 4–9).
- How to observe it: Send an HTTP POST request to `/api/coupon` or run test case `coupon_cap_applied_per_order`.
- Failure: Lines 5–8 accumulate the unconstrained discount across all orders and apply `std::min(total, cap_per_order_cents)` once to the total sum. For two ₹1,000 orders with 15% discount and an ₹200 cap per order, this produces only ₹200 (20,000 paise) total savings instead of capping each order individually.
- Expected: Apply `cap_per_order_cents` to each order independently before summing, granting ₹150 per ₹1,000 order for a total savings of ₹300 (30,000 paise).
