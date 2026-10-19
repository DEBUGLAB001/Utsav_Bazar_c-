# Utsav Bazaar - Diwali Mela & Gifting Companion

Utsav Bazaar is a high-performance C++ and web application built for festival guests at an artisan Diwali mela. It enables shoppers and festival attendees to curate personalized gift hampers within a rupee budget, plan conflict-free evening itineraries from scheduled cultural events, navigate illuminated walking trails through the mela grounds, make real-time reservations for limited-run artisan inventory, apply checkout discounts, and track order lifecycles alongside payment updates. The system couples a native C++ backend—powered by `httplib.h` and modern algorithm services—with a responsive, festive client dashboard that consumes real JSON API endpoints rather than mocking logic in the browser.

---

## 1. Application Overview

### Core Functionality

- **Artisan Gift Hamper Curation**: Evaluates artisan items (such as Kesar Mithai, Hand-painted Diyas, Silk Keepsake Scarves, and Marigold Tea Tins) against user-specified budgets ranging from ₹100 to ₹10,000, assembling a hamper that maximizes total festive "joy points" without exceeding cost limits.
- **Festival Evening Itinerary Planning**: Schedules non-overlapping cultural activities across the evening mela programme (including opening ceremonies, craft workshops, live folk music, and lantern walks), finding an optimal sequence of events for guests.
- **Illuminated Walking Trail Routing**: Models the mela layout as a network of festival stops (Jasmine Courtyard, Main Lantern Arch, Rangoli Steps, River of Lights) connected by illuminated pathways, computing the shortest travel time between landmarks.
- **Limited-Edition Artisan Inventory Reservations**: Tracks live inventory for scarce handmade goods (such as limited-run hand-thrown artisan diyas) and processes reservation requests safely in concurrent environments.
- **Promotional Coupon Discount Calculation**: Applies multi-order festive promotional codes (such as `DEEPA15` offering 15% off up to ₹200 per order) across gift orders during checkout, properly enforcing promotional discount caps.
- **Order Lifecycle & Payment Webhook Processing**: Manages end-to-end gift order states (`Pending`, `Paid`, `Cancelled`), supporting buyer cancellation and synchronizing asynchronous webhook notifications dispatched by external payment providers.

### Technology Stack

- **Runtime & Language**: C++17 (compiled with GCC / Clang / MSVC).
- **HTTP Server**: `httplib.h` (`cpp-httplib` v0.12+ single-header HTTP/HTTPS server library).
- **Data Serialization**: `json.hpp` (Nlohmann JSON for Modern C++ v3.x single-header library).
- **Concurrency & Multithreading**: C++ standard synchronization primitives (`<atomic>`, `<mutex>`, `<thread>`) and Windows threading APIs (`CreateThread`, `WaitForMultipleObjects`) under `_WIN32`.
- **Frontend Dashboard**: Semantic HTML5, Vanilla CSS3 (custom dark festival theme `#102b25` with gold typography, cards, and micro-interactions), and Vanilla JavaScript (Fetch API with relative endpoints).
- **Build System**: CMake 3.20+ with native Ninja or MinGW/Makefiles generator.
- **Test Runner**: Native C++ test executable (`tests/test_suite.cpp`) outputting structured JSON metrics.
- **Platform Dependencies**: Winsock (`ws2_32`) on Windows platforms.

### Build and Execution Commands

- **Configure and Build**:
  ```bash
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j2
  ```
- **Start the Server**:
  ```bash
  ./start.sh
  ```
  *Alternatively, run the compiled binary directly (default port `8080`, configurable via `$PORT`):*
  ```bash
  # Linux / macOS
  PORT=8080 ./build/server

  # Windows (PowerShell)
  $env:PORT="8080"; .\build\server.exe
  ```
- **Run the Test Suite**:
  ```bash
  # Via shell script
  bash ./tests/run_tests.sh

  # Or build and execute the native test binary directly
  cmake --build build --target challenge_tests
  ./build/challenge_tests
  ```

---

## 2. Debugging Challenge

Utsav Bazaar contains six confirmed defects across its core C++ algorithmic services, concurrency primitives, and state management logic. Because the frontend relies directly on real JSON responses from the backend, each bug manifests as incorrect calculations, race conditions, or invalid state transitions visible in both the automated test suite and the interactive web dashboard.

Your objective is to investigate the request and calculation flows across `src/service/`, resolve each underlying issue, and verify that the application satisfies all correctness guarantees.

### Reported Issues & Tasks

#### Issue 1: Suboptimal Gift Selection Under Budget Constraints

- **Reproduction**: Request a hamper recommendation for a budget of ₹1,800 (`POST /api/hamper` with `{"budget": 1800}`) or run the `gift_hamper_budget_optimization` test case with items: Kesar Mithai (cost: 700, joy: 95), Diya Set (cost: 500, joy: 80), Silk Scarf (cost: 1200, joy: 160), Marigold Tea (cost: 900, joy: 130).
- **Failure**: The service selects items based on a greedy value-per-rupee heuristic, picking Diya Set (500) and Marigold Tea (900) for a total cost of ₹1,400 and total joy of 210 points. It fails to identify superior combinations that fit within the ₹1,800 budget.
- **Expected**: The optimizer should find the globally optimal 0/1 knapsack combination within budget: Diya Set (cost 500, joy 80) and Silk Scarf (cost 1200, joy 160), yielding total cost 1700 (<= 1800) and a maximum joy of 240 points.

#### Issue 2: Incorrect Itinerary Scheduling Prioritizing Start Times Over Earliest Finish

- **Reproduction**: Generate an evening schedule from the festival events (`POST /api/itinerary`) or run `festival_itinerary_selection` with: Opening (start: 9, finish: 15), Workshop (start: 10, finish: 11), Folk Music (start: 11, finish: 12), Lantern Walk (start: 12, finish: 13).
- **Failure**: The scheduler sorts events by earliest start time. Consequently, it selects the long opening event (9:00 to 15:00), which blocks all subsequent events and yields only 1 scheduled activity.
- **Expected**: Interval scheduling should maximize non-overlapping events by prioritizing earliest finish times, successfully scheduling 3 distinct events (Workshop: 10–11, Folk: 11–12, Walk: 12–13).

#### Issue 3: Suboptimal Walking Route Calculation Using Unweighted Search Over Weighted Trails

- **Reproduction**: Find the walking route from Jasmine Courtyard (node 0) to Main Lantern Arch (node 1) via `POST /api/light-trail` or test `weighted_light_trail_route` where road 0->1 costs 10 min, road 0->2 costs 1 min, and road 2->1 costs 1 min.
- **Failure**: The pathfinder employs unweighted Breadth-First Search (BFS), selecting the direct single-hop edge 0 -> 1 with a travel time of 10 minutes.
- **Expected**: The algorithm should evaluate edge weights using Dijkstra's shortest path algorithm, discovering the route 0 -> 2 -> 1 with a total travel time of 2 minutes.

#### Issue 4: Race Condition in Concurrent Limited-Stock Diya Reservations

- **Reproduction**: Submit concurrent reservation requests (`POST /api/reservations` with `{"quantity": 1}`) when available inventory is 1, or execute the multi-threaded test `limited_stock_concurrent_reservations`.
- **Failure**: The ledger checks remaining capacity and stores the updated count in two separate, non-atomic operations separated by a delay. Concurrent threads both read capacity as available, resulting in overselling (both reservations return `accepted: true`).
- **Expected**: Stock reservation must be strictly atomic (e.g., via atomic compare-and-swap or mutex locking). Exactly one shopper must successfully reserve the final available piece, and the concurrent request must be rejected.

#### Issue 5: Delayed Payment Webhook Overwrites Order Cancellation State

- **Reproduction**: Create an order, cancel it (`POST /api/orders/cancel`), and then simulate an asynchronous payment provider success notification (`POST /api/orders/payment-update`), or run `late_payment_webhook_after_cancellation`.
- **Failure**: The payment webhook handler unconditionally overwrites the order status to `Paid`, reverting a finalized `Cancelled` order into an active paid order.
- **Expected**: An order in `Cancelled` status is in a terminal state and must remain `Cancelled`. Delayed payment confirmations for cancelled orders must be ignored or rejected without altering terminal status.

#### Issue 6: Global Coupon Discount Cap Applied Across Entire Batch Instead of Per Order

- **Reproduction**: Apply coupon `DEEPA15` (15% off, capped at ₹200 / 20,000 paise per order) to a checkout containing two separate ₹1,000 orders (`POST /api/coupon`) or execute `coupon_cap_applied_per_order`.
- **Failure**: The calculator sums the percentage discounts across all orders first and then applies the ₹200 cap once to the entire aggregate sum, granting only ₹200 (20,000 paise) total discount.
- **Expected**: The discount cap of ₹200 (20,000 paise) must apply independently to each eligible order. For two ₹1,000 orders (15% = ₹150 each), each order receives a ₹150 discount, resulting in a combined discount of ₹300 (30,000 paise).

---

## 3. Expected Behavior After Fixing Bugs

Once all six defects are corrected, the application and its test suite will exhibit the following verified behaviors:

1. **Optimal Hamper Composition**: Requesting a gift hamper with an ₹1,800 budget yields the maximum feasible joy of 240 points by choosing the Diya Set and Silk Scarf (total cost ₹1,700).
2. **Maximum Event Scheduling**: Generating an evening itinerary schedules 3 non-overlapping cultural experiences by ordering intervals according to earliest finish times.
3. **True Shortest Walking Trails**: Finding the light trail from Jasmine Courtyard to Main Lantern Arch routes via Rangoli Steps (distance: 2 min) rather than the direct unweighted hop (distance: 10 min).
4. **Thread-Safe Stock Control**: Concurrent attempts to reserve the last remaining artisan diya allow exactly one successful reservation, preventing inventory overbooking.
5. **Resilient Order State Machine**: An order marked `Cancelled` ignores late payment provider webhooks, remaining securely `Cancelled`.
6. **Accurate Per-Order Promotional Discounts**: Applying `DEEPA15` to two ₹1,000 orders grants ₹150 discount per order for a total savings of ₹300 (30,000 paise), correctly adhering to per-order caps.

### Test Suite Verification

Run the automated test runner to validate fixes:

```bash
bash tests/run_tests.sh
```

Or execute the compiled binary directly:

```bash
./build/challenge_tests
```

*Verification status:* The native test suite was executed against the initial repository codebase using `./build-clang/challenge_tests.exe`. All 6 tests failed with their respective assertion errors, confirming the presence of the six documented challenge defects.
#   U t s a v _ B a z a r _ c -  
 