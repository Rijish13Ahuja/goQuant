#include "core/order_book.hpp"
#include <algorithm>
#include <iostream>

namespace GoQuant
{

    OrderBook::OrderBook(const std::string &symbol) : symbol_(symbol) {}

    bool OrderBook::add_order(const Order &order)
    {
        std::lock_guard<std::mutex> lock(book_mutex_);

        Order new_order = order;
        new_order.status = OrderStatus::ACTIVE;
        add_to_book(new_order);

        std::cout << "Order added: " << new_order.order_id
                  << " " << (new_order.side == OrderSide::BUY ? "BUY" : "SELL")
                  << " " << new_order.quantity << " @ " << new_order.price << std::endl;

        return true;
    }

    bool OrderBook::cancel_order(const std::string &order_id)
    {
        std::lock_guard<std::mutex> lock(book_mutex_);

        auto it = order_lookup_.find(order_id);
        if (it == order_lookup_.end())
        {
            std::cout << "Cancel failed: Order " << order_id << " not found" << std::endl;
            return false;
        }

        remove_from_book(order_id);
        std::cout << "Order cancelled: " << order_id << std::endl;
        return true;
    }

    void OrderBook::add_to_book(Order &order)
    {
        auto &book = (order.side == OrderSide::BUY) ? bids_ : asks_;
        auto &level = book[order.price];

        level.push_back(order);

        order_lookup_[order.order_id] = {book.find(order.price), level.size() - 1};
    }

    void OrderBook::remove_from_book(const std::string &order_id)
    {
        auto it = order_lookup_.find(order_id);
        if (it == order_lookup_.end())
            return;

        auto &[price_it, index] = it->second;
        auto &orders = price_it->second;

        if (index < orders.size())
        {
            orders[index] = orders.back();
            orders.pop_back();
            if (!orders.empty() && index < orders.size())
            {
                order_lookup_[orders[index].order_id] = {price_it, index};
            }
        }

        if (orders.empty())
        {
            if (orders[0].side == OrderSide::BUY)
            {
                bids_.erase(price_it);
            }
            else
            {
                asks_.erase(price_it);
            }
        }

        order_lookup_.erase(order_id);
    }

    double OrderBook::get_best_bid() const
    {
        std::lock_guard<std::mutex> lock(book_mutex_);
        return bids_.empty() ? 0.0 : bids_.begin()->first;
    }

    double OrderBook::get_best_ask() const
    {
        std::lock_guard<std::mutex> lock(book_mutex_);
        return asks_.empty() ? 0.0 : asks_.begin()->first;
    }

    std::vector<std::pair<double, double>> OrderBook::get_bid_levels(size_t depth) const
    {
        std::lock_guard<std::mutex> lock(book_mutex_);
        std::vector<std::pair<double, double>> levels;

        size_t count = 0;
        for (const auto &[price, orders] : bids_)
        {
            if (count++ >= depth)
                break;
            double total_qty = 0;
            for (const auto &order : orders)
            {
                total_qty += order.quantity;
            }
            levels.emplace_back(price, total_qty);
        }

        return levels;
    }

    std::vector<std::pair<double, double>> OrderBook::get_ask_levels(size_t depth) const
    {
        std::lock_guard<std::mutex> lock(book_mutex_);
        std::vector<std::pair<double, double>> levels;

        size_t count = 0;
        for (const auto &[price, orders] : asks_)
        {
            if (count++ >= depth)
                break;
            double total_qty = 0;
            for (const auto &order : orders)
            {
                total_qty += order.quantity;
            }
            levels.emplace_back(price, total_qty);
        }

        return levels;
    }

}