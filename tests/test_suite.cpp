#include "json.hpp"
#include "service/coupon_calculator.hpp"
#include "service/hamper_optimizer.hpp"
#include "service/itinerary_planner.hpp"
#include "service/light_trail.hpp"
#include "service/payment_webhook.hpp"
#include "service/stock_ledger.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <thread>
#endif
#include <vector>
using json=nlohmann::json; using namespace utsav;
struct TestCase { std::string name; std::function<void()> run; };
#ifdef _WIN32
struct ReservationCheck { StockLedger* stock; bool* accepted; };
static DWORD WINAPI reservation_worker(LPVOID raw) {
 auto* check=static_cast<ReservationCheck*>(raw); *check->accepted=check->stock->reserve(1); return 0;
}
#endif
int main(){
 std::vector<TestCase> cases;
 cases.push_back({"gift_hamper_budget_optimization",[]{std::vector<Gift> g{{"Kesar",700,95},{"Diya",500,80},{"Scarf",1200,160},{"Tea",900,130}};if(maximize_hamper_joy(g,1800)!=240)throw std::runtime_error("Best feasible joy is 240.");}});
 cases.push_back({"festival_itinerary_selection",[]{std::vector<Event> e{{"Opening",9,15},{"Workshop",10,11},{"Folk",11,12},{"Walk",12,13}};if(select_event_count(e)!=3)throw std::runtime_error("Three non-overlapping events fit.");}});
 cases.push_back({"weighted_light_trail_route",[]{std::vector<std::vector<std::pair<int,int>>> g(3);g[0]={{1,10},{2,1}};g[2]={{1,1}};if(shortest_light_trail(g,0,1)!=2)throw std::runtime_error("Shortest weighted route costs 2.");}});
 cases.push_back({"limited_stock_concurrent_reservations",[]{StockLedger stock(1);bool a=false,b=false;
#ifdef _WIN32
 ReservationCheck first{&stock,&a},second{&stock,&b};HANDLE t1=CreateThread(nullptr,0,reservation_worker,&first,0,nullptr),t2=CreateThread(nullptr,0,reservation_worker,&second,0,nullptr);HANDLE workers[2]={t1,t2};WaitForMultipleObjects(2,workers,TRUE,INFINITE);CloseHandle(t1);CloseHandle(t2);
#else
 std::thread t1([&]{a=stock.reserve(1);}),t2([&]{b=stock.reserve(1);});t1.join();t2.join();
#endif
 if(static_cast<int>(a)+static_cast<int>(b)!=1)throw std::runtime_error("Exactly one shopper should reserve the last item.");}});
 cases.push_back({"late_payment_webhook_after_cancellation",[]{Order order{"UB-test",OrderStatus::Cancelled};apply_payment_webhook(order,true);if(order.status!=OrderStatus::Cancelled)throw std::runtime_error("Cancelled order must remain cancelled.");}});
 cases.push_back({"coupon_cap_applied_per_order",[]{if(coupon_discount_cents({{100000},{100000}},15,20000)!=30000)throw std::runtime_error("Two orders should receive a combined 30000 paise discount.");}});
 json report=json::object();int passed=0,failed=0;auto suite=std::chrono::steady_clock::now();
 for(const auto& test:cases){auto start=std::chrono::steady_clock::now();try{test.run();++passed;report[test.name]={{"Status","passed"},{"Error",""}};}catch(const std::exception& e){++failed;report[test.name]={{"Status","failed"},{"Error",e.what()}};}auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();report[test.name]["Execution time"]=std::to_string(ms)+"ms";}
 auto total=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-suite).count();
 report["Passed"]=passed;report["Failed"]=failed;report["Total bugs"]=static_cast<int>(cases.size());report["total"]=static_cast<int>(cases.size());report["passed"]=passed;report["failed"]=failed;report["Total Execution time"]=std::to_string(total)+"ms";
 std::cout<<report.dump(2)<<std::endl;return failed==0?0:1;
}
