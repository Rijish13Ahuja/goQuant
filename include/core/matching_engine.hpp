#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "order_book.hpp"
#include <unordered_map>
#include <memory>

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

    private:
        std::unordered_map<std::string, std::shared_ptr<OrderBook>> order_books_;
        std::mutex engine_mutex_;
        void match_order(Order &order, OrderBook &book);
    };

}

#endif