#include "core/matching_engine.hpp"
#include <iostream>

namespace GoQuant
{

    MatchingEngine::MatchingEngine()
    {
        add_symbol("BTC-USDT");
        add_symbol("ETH-USDT");
    }

    bool MatchingEngine::submit_order(Order order)
    {
        std::lock_guard<std::mutex> lock(engine_mutex_);

        auto book_it = order_books_.find(order.symbol);
        if (book_it == order_books_.end())
        {
            std::cout << "Error: Symbol " << order.symbol << " not supported" << std::endl;
            return false;
        }

        std::vector<Trade> trades;
        bool success = book_it->second->add_order(order, trades);

        for (const auto &trade : trades)
        {
            std::cout << "EXECUTED: " << trade.symbol << " " << trade.quantity
                      << " @ " << trade.price << " (" << trade.aggressor_side << ")" << std::endl;
        }

        return success;
    }

    bool MatchingEngine::cancel_order(const std::string &symbol, const std::string &order_id)
    {
        std::lock_guard<std::mutex> lock(engine_mutex_);

        auto book_it = order_books_.find(symbol);
        if (book_it == order_books_.end())
        {
            return false;
        }

        return book_it->second->cancel_order(order_id);
    }

    std::shared_ptr<OrderBook> MatchingEngine::get_order_book(const std::string &symbol)
    {
        std::lock_guard<std::mutex> lock(engine_mutex_);
        auto it = order_books_.find(symbol);
        return (it != order_books_.end()) ? it->second : nullptr;
    }

    void MatchingEngine::add_symbol(const std::string &symbol)
    {
        std::lock_guard<std::mutex> lock(engine_mutex_);
        if (order_books_.find(symbol) == order_books_.end())
        {
            order_books_[symbol] = std::make_shared<OrderBook>(symbol);
            std::cout << "Added symbol: " << symbol << std::endl;
        }
    }

}