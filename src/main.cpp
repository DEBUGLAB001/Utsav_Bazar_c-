#include "httplib.h"
#include "json.hpp"
#include "service/coupon_calculator.hpp"
#include "service/hamper_optimizer.hpp"
#include "service/itinerary_planner.hpp"
#include "service/light_trail.hpp"
#include "service/payment_webhook.hpp"
#include "service/stock_ledger.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <thread>
#endif
using json=nlohmann::json;
using namespace utsav;

namespace {
const std::vector<Gift> gifts{{"Kesar Mithai Box",700,95},{"Hand-painted Diya Set",500,80},{"Silk Keepsake Scarf",1200,160},{"Marigold Tea Tin",900,130}};
const std::vector<Event> events{{"Grand Mela Opening",17,23},{"Rangoli Workshop",18,19},{"Live Folk Music",19,20},{"Lantern Walk",20,21}};
const std::vector<std::string> stops{"Jasmine Courtyard","Main Lantern Arch","Rangoli Steps","River of Lights"};
const std::vector<std::vector<std::pair<int,int>>> roads{{{1,10},{2,1}},{{3,2}},{{1,1},{3,8}},{}};
StockLedger diya_stock(1);
Order sample_order{"UB-4821",OrderStatus::Pending};
bool order_placed=false;
int order_amount_cents=200000;
std::mutex order_mutex;

json body_json(const httplib::Request& req) {
    try { return req.body.empty()?json::object():json::parse(req.body); }
    catch(...) { return json::object(); }
}
json hamper_result(int budget) {
    const auto plan=build_hamper(gifts,budget);
    json items=json::array();
    for(int index:plan.gift_indexes) items.push_back({{"name",gifts[index].name},{"price_cents",gifts[index].cost*100},{"joy",gifts[index].joy}});
    return {{"budget_cents",budget*100},{"total_cents",plan.total_cost*100},{"joy",plan.total_joy},{"items",items}};
}
json itinerary_result() {
    const auto indexes=plan_itinerary(events); json selected=json::array();
    for(int index:indexes) selected.push_back({{"name",events[index].name},{"start",events[index].start},{"finish",events[index].finish}});
    return {{"selected_count",selected.size()},{"events",selected}};
}
json programme_result() {
    json programme=json::array();
    for(const auto& event:events) programme.push_back({{"name",event.name},{"start",event.start},{"finish",event.finish}});
    return programme;
}
json route_result() {
    const auto route=find_light_trail(roads,0,1); json path=json::array();
    for(int index:route.path) path.push_back(stops[index]);
    return {{"distance",route.distance},{"path",path}};
}
json make_state() {
    const auto hamper=hamper_result(1800); const auto itinerary=itinerary_result(); const auto route=route_result();
    json gift_list=json::array();
    for(const auto& gift:gifts) gift_list.push_back({{"name",gift.name},{"price_cents",gift.cost*100},{"joy",gift.joy}});
    std::string order_status; int amount_cents; bool placed;
    { std::lock_guard<std::mutex> lock(order_mutex); order_status=status_name(sample_order.status); placed=order_placed; amount_cents=order_amount_cents; }
    return {
      {"service_status","ONLINE"},{"brand","Utsav Bazaar"},{"season","Diwali Mela · 2026"},
      {"gifts",gift_list},
      {"hamper",hamper},{"programme",programme_result()},{"itinerary",itinerary},{"route",route},
      {"stock",{{"product","Limited-run artisan diya"},{"available",diya_stock.available()},{"initial",1}}},
      {"order",{{"id",sample_order.id},{"status",order_status},{"amount_cents",amount_cents},{"placed",placed}}}
    };
}
}

int main() {
    httplib::Server server;
    server.set_mount_point("/","./public");
    server.Get("/api/health",[](const httplib::Request&,httplib::Response& res){res.set_content(R"({"status":"ok"})","application/json");});
    server.Get("/api/state",[](const httplib::Request&,httplib::Response& res){res.set_content(make_state().dump(),"application/json");});
    server.Post("/api/hamper",[](const httplib::Request& req,httplib::Response& res){
        const auto body=body_json(req); const int budget=body.value("budget",1800);
        if(budget<100||budget>10000){res.status=400;res.set_content(R"({"error":"Choose a budget from ₹100 to ₹10,000."})","application/json");return;}
        res.set_content(hamper_result(budget).dump(),"application/json");
    });
    server.Post("/api/itinerary",[](const httplib::Request&,httplib::Response& res){res.set_content(itinerary_result().dump(),"application/json");});
    server.Post("/api/light-trail",[](const httplib::Request&,httplib::Response& res){res.set_content(route_result().dump(),"application/json");});
    server.Post("/api/reservations",[](const httplib::Request& req,httplib::Response& res){
        const int quantity=body_json(req).value("quantity",1); const bool accepted=diya_stock.reserve(quantity);
        res.set_content(json{{"accepted",accepted},{"available",diya_stock.available()},{"product","Limited-run artisan diya"}}.dump(),"application/json");
    });
    server.Post("/api/coupon",[](const httplib::Request&,httplib::Response& res){
        const int discount=coupon_discount_cents({{100000},{100000}},15,20000);
        res.set_content(json{{"code","DEEPA15"},{"discount_cents",discount},{"total_cents",200000-discount},{"orders",2}}.dump(),"application/json");
    });
    server.Post("/api/orders/checkout",[](const httplib::Request& req,httplib::Response& res){
        const auto body=body_json(req); const bool use_coupon=body.value("coupon",false);
        std::lock_guard<std::mutex> lock(order_mutex);
        order_amount_cents=use_coupon?200000-coupon_discount_cents({{100000},{100000}},15,20000):200000;
        order_placed=true; sample_order.status=OrderStatus::Pending;
        res.set_content(json{{"id",sample_order.id},{"status",status_name(sample_order.status)},{"amount_cents",order_amount_cents},{"placed",true}}.dump(),"application/json");
    });
    server.Post("/api/orders/cancel",[](const httplib::Request&,httplib::Response& res){
        std::lock_guard<std::mutex> lock(order_mutex);
        if(!order_placed){res.status=409;res.set_content(json{{"message","Place your gift orders before cancelling."}}.dump(),"application/json");return;}
        if(sample_order.status!=OrderStatus::Pending){res.status=409;res.set_content(json{{"status",status_name(sample_order.status)},{"message","This order can no longer be cancelled."}}.dump(),"application/json");return;}
        sample_order.status=OrderStatus::Cancelled;
        res.set_content(json{{"id",sample_order.id},{"status",status_name(sample_order.status)}}.dump(),"application/json");
    });
    server.Post("/api/orders/payment-update",[](const httplib::Request&,httplib::Response& res){
        std::lock_guard<std::mutex> lock(order_mutex); if(!order_placed){res.status=409;res.set_content(json{{"message","There is no active order to update."}}.dump(),"application/json");return;} apply_payment_webhook(sample_order,true);
        res.set_content(json{{"id",sample_order.id},{"status",status_name(sample_order.status)}}.dump(),"application/json");
    });
    server.Post("/api/test",[](const httplib::Request&,httplib::Response& res){
#ifdef _WIN32
        FILE* pipe=_popen("bash tests/run_tests.sh 2>NUL","r");
#else
        FILE* pipe=popen("./tests/run_tests.sh 2>/dev/null","r");
#endif
        if(!pipe){res.status=500;res.set_content(R"({"error":"Could not start test runner"})","application/json");return;}
        std::string output;char buffer[4096];while(fgets(buffer,sizeof(buffer),pipe))output+=buffer;
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        res.set_content(output,"application/json");
    });
    int port=8080;if(const char* value=std::getenv("PORT"))port=std::atoi(value);
    std::cout<<"[Server] Listening on 0.0.0.0:"<<port<<std::endl;
    return server.listen("0.0.0.0",port)?0:1;
}
