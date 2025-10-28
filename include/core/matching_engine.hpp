#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "order_book.hpp"
#include "advanced_orders.hpp"
#include "fees/fee_calculator.hpp"
#include "utils/performance_counter.hpp"
#include <unordered_map>
#include <memory>
#include <functional>

namespace GoQuant {

class MatchingEngine {
public:
    MatchingEngine();
    
    bool submit_order(Order order);
    bool cancel_order(const std::string& symbol, const std::string& order_id);
    
    std::shared_ptr<OrderBook> get_order_book(const std::string& symbol);
    void add_symbol(const std::string& symbol);
    
    void set_trade_callback(std::function<void(const Trade&)> callback) {
        trade_callback_ = callback;
    }

    AdvancedOrderManager& get_advanced_order_manager() { return advanced_order_manager_; }
    FeeCalculator& get_fee_calculator() { return fee_calculator_; }
    
    void update_market_price(const std::string& symbol, double price);
    
    uint64_t get_orders_processed() const { return orders_processed_; }
    double get_throughput_ops() const;

private:
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;
    std::mutex engine_mutex_;
    std::function<void(const Trade&)> trade_callback_;
    
    AdvancedOrderManager advanced_order_manager_;
    FeeCalculator fee_calculator_;
    ThroughputCounter throughput_counter_;
    std::atomic<uint64_t> orders_processed_{0};
    
    void on_trade_executed(const Trade& trade) {
        if (trade_callback_) {
            trade_callback_(trade);
        }
    }
};

} 

#endif