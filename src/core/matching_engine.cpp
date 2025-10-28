#include "core/matching_engine.hpp"
#include <iostream>

namespace GoQuant {

MatchingEngine::MatchingEngine() : fee_calculator_(FeeStructure(0.001, 0.002)) {
    add_symbol("BTC-USDT");
    add_symbol("ETH-USDT");
    
    advanced_order_manager_.set_order_callback([this](const Order& order) {
        this->submit_order(order);
    });
}

bool MatchingEngine::submit_order(Order order) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto book_it = order_books_.find(order.symbol);
    if (book_it == order_books_.end()) {
        std::cout << "Error: Symbol " << order.symbol << " not supported" << std::endl;
        return false;
    }
    
    book_it->second->set_trade_callback([this](const Trade& trade) {
        on_trade_executed(trade);
    });
    
    throughput_counter_.increment();
    orders_processed_++;
    
    std::vector<Trade> trades;
    bool success = book_it->second->add_order(order, trades);
    
    for (const auto& trade : trades) {
        std::cout << "EXECUTED: " << trade.symbol << " " << trade.quantity 
                  << " @ " << trade.price << " (" << trade.aggressor_side << ")" << std::endl;
        on_trade_executed(trade);
    }
    
    return success;
}

bool MatchingEngine::cancel_order(const std::string& symbol, const std::string& order_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto book_it = order_books_.find(symbol);
    if (book_it == order_books_.end()) {
        return false;
    }
    
    return book_it->second->cancel_order(order_id);
}

std::shared_ptr<OrderBook> MatchingEngine::get_order_book(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second : nullptr;
}

void MatchingEngine::add_symbol(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    if (order_books_.find(symbol) == order_books_.end()) {
        order_books_[symbol] = std::make_shared<OrderBook>(symbol);
        std::cout << "Added symbol: " << symbol << std::endl;
    }
}

void MatchingEngine::update_market_price(const std::string& symbol, double price) {
    advanced_order_manager_.check_triggers(symbol, price);
}

double MatchingEngine::get_throughput_ops() const {
    return throughput_counter_.get_throughput_per_second();
}

} 