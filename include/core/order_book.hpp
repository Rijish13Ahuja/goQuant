#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include "order_types.hpp"
#include "trade.hpp"
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <queue>

namespace GoQuant
{

    class OrderBook
    {
    public:
        explicit OrderBook(const std::string &symbol);

        bool add_order(Order &order, TradeCallback trade_cb);
        bool cancel_order(const std::string &order_id);
        bool modify_order(const std::string &order_id, double new_quantity);

        double get_best_bid() const;
        double get_best_ask() const;
        std::string get_symbol() const { return symbol_; }

        std::vector<std::pair<double, double>> get_bid_levels(size_t depth = 10) const;
        std::vector<std::pair<double, double>> get_ask_levels(size_t depth = 10) const;

        size_t get_total_orders() const { return order_lookup_.size(); }

    private:
        std::string symbol_;

        std::map<double, std::deque<Order>, std::greater<double>> bids_;
        std::map<double, std::deque<Order>> asks_;

        struct OrderLocation
        {
            bool is_bid;
            std::map<double, std::deque<Order>>::iterator price_it;
            size_t order_index;
        };
        std::unordered_map<std::string, OrderLocation> order_lookup_;

        mutable std::mutex book_mutex_;

        void match_order(Order &order, TradeCallback trade_cb);
        bool try_match_market_order(Order &order, TradeCallback trade_cb);
        bool try_match_limit_order(Order &order, TradeCallback trade_cb);
        bool try_match_ioc_order(Order &order, TradeCallback trade_cb);
        bool try_match_fok_order(Order &order, TradeCallback trade_cb);

        void execute_trade(Order &taker, Order &maker, double price,
                           double quantity, TradeCallback trade_cb);

        void add_to_book(Order &order);
        void remove_from_book(const std::string &order_id);
        bool can_match_buy(double price) const;
        bool can_match_sell(double price) const;
        double get_matching_price(const Order &taker, const Order &maker) const;
    };

}

#endif