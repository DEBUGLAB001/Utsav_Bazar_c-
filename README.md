# Utsav Bazaar — Diwali Mela & Gifting Companion

Utsav Bazaar is an interactive festival fair companion designed for guests celebrating Diwali. It allows visitors to plan their mela experience by assembling optimized gift hampers, building conflict-free evening event schedules, discovering illuminated walking routes, reserving scarce artisan items, and managing checkout orders with promotional discounts.

## 1. Application Overview

### Core Functionality

- **Artisan Gift Hamper Curation**: Assembles a tailored collection of festival gifts that maximizes joy points within a shopper's chosen rupee budget.
- **Evening Event Itinerary Planning**: Selects a schedule of non-overlapping cultural activities across the evening, allowing guests to attend as many performances and workshops as possible.
- **Illuminated Trail Navigation**: Determines the shortest walking route between festival landmarks across illuminated fairground pathways.
- **Handcrafted Diya Reservations**: Manages real-time reservations for limited-run artisan diyas, ensuring fair allocation without overselling scarce inventory.
- **Promotional Coupon Discounts**: Computes order discounts using festival coupon codes, applying discount caps per individual order.
- **Order Lifecycle Management**: Handles gift orders through pending, paid, and cancelled stages while updating order statuses securely.

## 2. Debugging Challenge

Utsav Bazaar contains six defects across its gift optimization, scheduling, navigation, inventory, payment, and checkout calculations. Because the web dashboard communicates with the application server, these bugs result in suboptimal planning, race conditions, and incorrect discount totals. Your goal is to investigate each problem and restore correct behavior.

### Reported Issues & Tasks:

Issue 1: Suboptimal Gift Selection Under Budget Constraints

- User Symptom: When generating a gift hamper with an ₹1,800 budget, the application chooses items worth only 210 joy points instead of the best combination worth 240 joy points.
- Task: Ensure the hamper builder selects the combination of gifts that yields the highest total joy without exceeding the customer's budget.

Issue 2: Incorrect Itinerary Scheduling Prioritizing Start Times

- User Symptom: Requesting an evening schedule results in a single lengthy opening ceremony being booked, blocking all other workshops and performances for the rest of the evening.
- Task: Update the scheduling logic so attendees receive an itinerary with the maximum number of non-overlapping activities.

Issue 3: Suboptimal Walking Route Calculation on Weighted Trails

- User Symptom: The route planner recommends a 10-minute direct walking trail instead of a quicker 2-minute route along illuminated side paths.
- Task: Fix the route finder to evaluate actual walking times and find the true fastest path between festival landmarks.

Issue 4: Race Condition in Concurrent Stock Reservations

- User Symptom: When two shoppers simultaneously attempt to reserve the last available artisan diya, both reservations succeed and oversell the limited stock.
- Task: Enforce atomic stock reservations so only one customer successfully reserves the final item when inventory reaches capacity.

Issue 5: Delayed Payment Webhook Overwrites Order Cancellation

- User Symptom: When a customer cancels their pending gift order, a late payment notification from the payment provider reactivates the order back to paid status.
- Task: Ensure cancelled orders remain permanently cancelled even if late payment updates are received.

Issue 6: Global Coupon Discount Cap Applied Across Entire Batch

- User Symptom: Applying festival coupon `DEEPA15` to two separate ₹1,000 orders provides only ₹200 total discount instead of applying the discount limit per order for ₹300 total savings.
- Task: Apply promotional discount limits separately to each qualifying order in a multi-order checkout.

## 3. Expected Behavior After Fixing Bugs

1. Budget-based hamper curation returns the optimal gift combination (Diya Set and Silk Scarf) delivering 240 joy points for an ₹1,800 budget.
2. Itinerary planning schedules 3 non-overlapping cultural activities during the festival evening.
3. Walking navigation selects the shortest 2-minute route between Jasmine Courtyard and Main Lantern Arch.
4. Concurrent inventory reservations permit only one shopper to secure the last remaining artisan diya.
5. Cancelled orders ignore delayed payment provider notifications and remain cancelled.
6. Promotional coupon calculations apply the discount cap independently per order, granting ₹300 total discount across two ₹1,000 orders.

Once all issues are resolved, Utsav Bazaar will reliably provide accurate festival recommendations, fair inventory control, and correct order management.
