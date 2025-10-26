#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "order_book.hpp"
#include "order_types.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace GoQuant
{

    class MatchingEngine
    {
    public:
        MatchingEngine();

        bool submit_order(Order order);
        bool cancel_order(const std::string &symbol, const std::string &order_id);

        std::shared_ptr<OrderBook> get_order_book(const std::string &symbol);
        void add_symbol(const std::string &symbol);
        void set_trade_callback(TradeCallback cb) { trade_callback_ = cb; }
        void set_order_update_callback(OrderUpdateCallback cb) { order_update_callback_ = cb; }

        size_t get_total_trades() const { return total_trades_; }
        size_t get_total_orders() const;

    private:
        std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;
        std::mutex engine_mutex_;

        TradeCallback trade_callback_;
        OrderUpdateCallback order_update_callback_;

        std::atomic<size_t> total_trades_{0};

        void on_trade_executed(const Trade &trade);
        void on_order_updated(const Order &order);
    };

}

#endif