# Utsav Bazaar - Technical Documentation

## Description

Utsav Bazaar is a C++17 web application and backend algorithm suite created for guests and administrators of an artisan Diwali mela (festival fair). It assists visitors in planning their festival experience by curating optimized gift hampers, scheduling non-overlapping cultural activities, computing shortest-path walking routes across illuminated festival grounds, reserving limited-edition handcrafted artisan goods, calculating promotional order discounts, and managing order lifecycles alongside asynchronous payment updates.

### Primary Workflows & Architecture

- **Client Dashboard**: A responsive single-page web dashboard served from `public/` that interacts with backend JSON APIs via HTTP `fetch` without client-side mock fallback.
- **HTTP Routing & API Gateway**: `src/main.cpp` initializes an embedded multi-threaded HTTP server using `httplib.h` (`cpp-httplib`), binding to `0.0.0.0:${PORT:-8080}`. It routes requests, deserializes JSON payloads using `json.hpp` (Nlohmann JSON), coordinates domain services, and manages shared state.
- **Domain Services**: Isolated algorithmic and business logic modules located in `src/service/` and declared in `include/service/`:
  - `hamper_optimizer`: 0/1 knapsack solver for budget-to-joy maximization.
  - `itinerary_planner`: Interval scheduling algorithm for maximizing attending cultural events.
  - `light_trail`: Graph shortest-path solver across the festival map.
  - `stock_ledger`: Inventory controller managing concurrent reservations for scarce artisan pieces.
  - `payment_webhook`: Order state machine reconciling asynchronous external payment notifications.
  - `coupon_calculator`: Discount engine evaluating percentage promotions and per-order discount caps.
- **Persistence & State Management**: State is held in-memory within `src/main.cpp` and `StockLedger` instances using thread synchronization primitives (`std::mutex`, `std::lock_guard`, `std::atomic<int>`). Domain entities (`Gift`, `Event`, `Order`, `HamperPlan`, `TrailPlan`) are defined as plain C++ structs in `include/model/domain.hpp`.
- **Runtime & Deployment**: Packaged with a CMake build system (`CMakeLists.txt`), a development watcher script with incremental recompilation (`start.sh`), and an automated native test runner (`tests/run_tests.sh` and `tests/test_suite.cpp`).

---

## Repository Structure

```text
utsav-bazaar/
├── CMakeLists.txt              CMake build configuration for core services, server, and tests
├── challenge.json              Challenge runtime metadata specifying build, start, and test commands
├── start.sh                    Development watcher script that rebuilds C++ server on file changes
├── README.md                   Candidate-facing overview, technology guide, and debugging challenge
├── AI.md                       Technical repository architecture, implementation details, and bug locations
├── include/
│   ├── httplib.h               Single-header C++11/C++17 HTTP/HTTPS server and client library
│   ├── json.hpp                Single-header JSON for Modern C++ (nlohmann::json)
│   ├── model/
│   │   └── domain.hpp          Core domain data models (Gift, Event, Order, HamperPlan, TrailPlan)
│   └── service/
│       ├── coupon_calculator.hpp  Declarations for promotional discount calculations
│       ├── hamper_optimizer.hpp   Declarations for knapsack budget-joy hamper builder
│       ├── itinerary_planner.hpp  Declarations for interval scheduling itinerary planner
│       ├── light_trail.hpp        Declarations for illuminated walking trail pathfinding
│       ├── payment_webhook.hpp    Declarations for order payment status webhook handler
│       └── stock_ledger.hpp       Declaration of thread-safe StockLedger class
├── src/
│   ├── main.cpp                HTTP server entry point, API route handlers, and in-memory state
│   └── service/
│       ├── coupon_calculator.cpp  Implementation of coupon discount calculation
│       ├── hamper_optimizer.cpp   Implementation of hamper curation logic
│       ├── itinerary_planner.cpp  Implementation of festival event scheduling logic
│       ├── light_trail.cpp        Implementation of walking trail routing logic
│       ├── payment_webhook.cpp    Implementation of payment webhook order transition logic
│       └── stock_ledger.cpp       Implementation of inventory reservation ledger
├── public/
│   ├── index.html              Single-page application dashboard layout and markup
│   ├── css/
│   │   └── style.css           Festive styling, layout grid, typography, and responsive rules
│   └── js/
│       └── app.js              Client-side controller calling /api endpoints and updating DOM
└── tests/
    ├── run_tests.sh            Shell script compiling and executing challenge_tests binary
    └── test_suite.cpp          Native C++ test runner executing unit tests and emitting JSON report
```

---

## Bugs and Bug Locations

### 1. Suboptimal Gift Selection Under Budget Constraints

- **Bug location:** `src/service/hamper_optimizer.cpp` in `utsav::build_hamper(const std::vector<Gift>& gifts, int budget)` (lines 5–19) and `utsav::maximize_hamper_joy` (line 20).
- **How to observe it:**
  - Send an HTTP POST request to `/api/hamper` with body `{"budget": 1800}`.
  - Or run the native test suite: case `gift_hamper_budget_optimization`.
- **Failure:** The algorithm uses a greedy ratio heuristic (`joy / cost`), sorting items in descending order of efficiency and packing them until the budget is depleted. For the test dataset (`Kesar` [700 cost, 95 joy], `Diya` [500 cost, 80 joy], `Scarf` [1200 cost, 160 joy], `Tea` [900 cost, 130 joy]) with an ₹1,800 budget, the greedy strategy selects `Diya` (ratio 0.16) and `Tea` (ratio 0.144), consuming ₹1,400 with a total joy of 210 points. It skips `Scarf` (cost 1200 > remaining 400), failing to identify that `Diya` + `Scarf` costs ₹1,700 and yields 240 joy points.
- **Expected:** The selection must implement 0/1 knapsack dynamic programming (or branch-and-bound) to find the combination maximizing total joy subject to $\sum \text{cost}_i \le \text{budget}$. For budget 1800, it should return items with total joy 240 (`Diya` and `Scarf`).
- **Execution Path:** Client clicks `#build-hamper` in `public/js/app.js` -> calls `api('hamper', {budget})` -> `server.Post("/api/hamper", ...)` in `src/main.cpp:79` -> calls `hamper_result(budget)` -> invokes `utsav::build_hamper` in `src/service/hamper_optimizer.cpp`.

### 2. Incorrect Itinerary Scheduling Prioritizing Start Times Over Earliest Finish

- **Bug location:** `src/service/itinerary_planner.cpp` in `utsav::plan_itinerary(const std::vector<Event>& events)` (lines 5–13) and `utsav::select_event_count` (line 14).
- **How to observe it:**
  - Send an HTTP POST request to `/api/itinerary`.
  - Or run the native test suite: case `festival_itinerary_selection`.
- **Failure:** Line 8 sorts events by start time: `std::sort(sorted.begin(), sorted.end(), [&](int a, int b) { return events[a].start < events[b].start; });`. When presented with an opening ceremony that starts first but runs for many hours (e.g., 9:00 to 15:00 in tests, or 17:00 to 23:00 in the dashboard), the greedy earliest-start selection picks this large interval first. This single event overlaps with and blocks all subsequent shorter events, resulting in only 1 event scheduled instead of 3.
- **Expected:** In activity selection / interval scheduling, the optimal greedy strategy must sort intervals by earliest finish time (`events[a].finish < events[b].finish`). This greedily leaves the maximum remaining time available for subsequent events, selecting 3 non-overlapping events (`Workshop` 10–11, `Folk` 11–12, and `Walk` 12–13).
- **Execution Path:** Client triggers `#plan-evening` in `public/js/app.js` -> calls `api('itinerary')` -> `server.Post("/api/itinerary", ...)` in `src/main.cpp:84` -> calls `itinerary_result()` -> invokes `utsav::plan_itinerary` in `src/service/itinerary_planner.cpp`.

### 3. Suboptimal Walking Route Calculation Using Unweighted Search Over Weighted Trails

- **Bug location:** `src/service/light_trail.cpp` in `utsav::find_light_trail(const std::vector<std::vector<std::pair<int,int>>>& graph, int start, int goal)` (lines 6–24) and `utsav::shortest_light_trail` (line 25).
- **How to observe it:**
  - Send an HTTP POST request to `/api/light-trail`.
  - Or run the native test suite: case `weighted_light_trail_route`.
- **Failure:** The function implements an unweighted Breadth-First Search (BFS) using a FIFO `std::queue<int>` (lines 10–22). While BFS finds the path with the fewest edge hops, it ignores edge weights when updating distances: the first time a neighbor is encountered, `distance[edge.first]` is set and never relaxed if a lower-cost path is found later. For a graph where edge $0 \to 1$ costs 10 min, but path $0 \to 2 \to 1$ costs $1 + 1 = 2$ min, BFS visits node 1 immediately along edge $0 \to 1$, returning a distance of 10 instead of 2.
- **Expected:** The function should use Dijkstra's algorithm with a min-priority queue (`std::priority_queue`) to relax edge distances and determine the true minimum-weight path $0 \to 2 \to 1$ with total distance 2.
- **Execution Path:** Client clicks `#find-route` in `public/js/app.js` -> calls `api('light-trail')` -> `server.Post("/api/light-trail", ...)` in `src/main.cpp:85` -> calls `route_result()` -> invokes `utsav::find_light_trail` in `src/service/light_trail.cpp`.

### 4. Race Condition in Concurrent Limited-Stock Diya Reservations

- **Bug location:** `src/service/stock_ledger.cpp` in `utsav::StockLedger::reserve(int quantity)` (lines 10–22).
- **How to observe it:**
  - Execute concurrent HTTP POST requests to `/api/reservations` with `{"quantity": 1}` when `available == 1`.
  - Or run the native test suite: case `limited_stock_concurrent_reservations`.
- **Failure:** `reserve()` checks `current + quantity > capacity_` using `reserved_.load()` (line 12), then pauses (`Sleep(8)` on Windows / `sleep_for` on POSIX), and finally calls `reserved_.store(current + quantity)` (line 20). Because load, validation, and store are not performed atomically, two threads executing concurrently when capacity is 1 both observe `current = 0`, pass the check, and both store `0 + 1 = 1`. Both calls return `true`, overselling the inventory.
- **Expected:** Reservation must atomically validate and increment the reserved count. This can be achieved with a CAS loop on `std::atomic<int>::compare_exchange_weak` / `compare_exchange_strong` or with a `std::mutex`. When remaining stock is 1, only one thread must succeed and return `true`; subsequent or racing requests exceeding capacity must return `false`.
- **Execution Path:** Client clicks `#reserve-diya` in `public/js/app.js` -> calls `api('reservations', {quantity: 1})` -> `server.Post("/api/reservations", ...)` in `src/main.cpp:86` -> calls `diya_stock.reserve(quantity)` on the global `StockLedger` instance.

### 5. Delayed Payment Webhook Overwrites Order Cancellation State

- **Bug location:** `src/service/payment_webhook.cpp` in `utsav::apply_payment_webhook(Order& order, bool payment_succeeded)` (lines 3–7).
- **How to observe it:**
  - In the web dashboard under "Orders & checkout", click "Place these gift orders", click "Cancel this order", and wait 1.8 seconds for the asynchronous webhook trigger (`/api/orders/payment-update`).
  - Or run the native test suite: case `late_payment_webhook_after_cancellation`.
- **Failure:** Line 6 contains `order.status = OrderStatus::Paid;` with no guard checking the current status of `order`. If the customer already cancelled the order (`OrderStatus::Cancelled`), receiving a late success webhook from the payment gateway unconditionally overwrites `order.status` to `OrderStatus::Paid`.
- **Expected:** Order state transitions must respect terminal statuses: once an order transitions to `OrderStatus::Cancelled`, late-arriving payment webhooks must not alter the status. The handler should only transition orders from `OrderStatus::Pending` to `OrderStatus::Paid`.
- **Execution Path:** Customer cancels order via `#cancel-order` in `public/js/app.js` -> calls `/api/orders/cancel` (sets status to `OrderStatus::Cancelled`) -> `app.js` schedules a delayed call (1.8s) to `/api/orders/payment-update` -> `server.Post("/api/orders/payment-update", ...)` in `src/main.cpp:108` -> invokes `utsav::apply_payment_webhook(sample_order, true)` in `src/service/payment_webhook.cpp`.

### 6. Global Coupon Discount Cap Applied Across Entire Batch Instead of Per Order

- **Bug location:** `src/service/coupon_calculator.cpp` in `utsav::coupon_discount_cents(const std::vector<CouponOrder>& orders, int percent, int cap_per_order_cents)` (lines 4–9).
- **How to observe it:**
  - In the web dashboard under "Orders & checkout", click "Apply code" for coupon `DEEPA15`.
  - Or run the native test suite: case `coupon_cap_applied_per_order`.
- **Failure:** Lines 5–8 accumulate the uncapped discount across all orders (`total += order.subtotal_cents * percent / 100`) and apply `std::min(total, cap_per_order_cents)` once at the very end. For two orders of ₹1,000 (100,000 paise) each, with 15% discount and a ₹200 (20,000 paise) cap per order, the function calculates $15,000 + 15,000 = 30,000$ paise and caps it to $20,000$ paise (₹200) total.
- **Expected:** The cap `cap_per_order_cents` must be applied per individual order: `std::min(order.subtotal_cents * percent / 100, cap_per_order_cents)`. For the two ₹1,000 orders, each order should receive a ₹150 (15,000 paise) discount, producing a total combined discount of ₹300 (30,000 paise).
- **Execution Path:** Client clicks `#apply-coupon` in `public/js/app.js` -> calls `api('coupon')` -> `server.Post("/api/coupon", ...)` in `src/main.cpp:90` -> invokes `utsav::coupon_discount_cents({{100000}, {100000}}, 15, 20000)` in `src/service/coupon_calculator.cpp`.

---

## Expected Behaviour After Fixing All Bugs

When all six implementation bugs are properly resolved, the system will operate with the following verified behaviors:

- **Hamper Optimization**: `utsav::build_hamper` and `utsav::maximize_hamper_joy` return the true 0/1 knapsack optimum (240 joy for ₹1,800 budget with items Diya Set + Silk Scarf).
- **Itinerary Planning**: `utsav::plan_itinerary` and `utsav::select_event_count` select 3 non-overlapping festival events by sorting on finish times.
- **Light Trail Navigation**: `utsav::find_light_trail` and `utsav::shortest_light_trail` use Dijkstra's algorithm to discover the 2-minute route $0 \to 2 \to 1$.
- **Concurrent Stock Ledger**: `StockLedger::reserve` guarantees atomic check-and-reserve semantics, permitting only one thread to reserve the final item and preventing overselling.
- **Payment Webhook Reconciliation**: `utsav::apply_payment_webhook` preserves the terminal `OrderStatus::Cancelled` status against delayed webhook events.
- **Promotional Coupon Application**: `utsav::coupon_discount_cents` evaluates discount caps per order, yielding 30,000 paise (₹300) in savings across two ₹1,000 orders.

### Automated Test Execution

The automated test runner is invoked using:

```bash
bash tests/run_tests.sh
```

Or by directly running:

```bash
cmake --build build --target challenge_tests
./build/challenge_tests
```

**Verification Status:** Executed `./build-clang/challenge_tests.exe` against the unpatched codebase. All 6 test cases currently fail with specific, reproducible assertion errors matching the documented issues. No application source code or tests were modified during this documentation task.
