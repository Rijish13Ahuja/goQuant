#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include "order_types.hpp"
#include <map>
#include <vector>
#include <memory>
#include <mutex>

namespace GoQuant
{

    class OrderBook
    {
    public:
        explicit OrderBook(const std::string &symbol);
        bool add_order(const Order &order);
        bool cancel_order(const std::string &order_id);
        bool modify_order(const std::string &order_id, double new_quantity);
        double get_best_bid() const;
        double get_best_ask() const;
        std::string get_symbol() const { return symbol_; }
        std::vector<std::pair<double, double>> get_bid_levels(size_t depth = 10) const;
        std::vector<std::pair<double, double>> get_ask_levels(size_t depth = 10) const;

    private:
        std::string symbol_;
        std::map<double, std::vector<Order>, std::greater<double>> bids_;
        std::map<double, std::vector<Order>> asks_;
        std::unordered_map<std::string,
                           std::pair<std::map<double, std::vector<Order>>::iterator, size_t>>
            order_lookup_;

        mutable std::mutex book_mutex_;

        void add_to_book(Order &order);
        void remove_from_book(const std::string &order_id);
    };

}

#endif