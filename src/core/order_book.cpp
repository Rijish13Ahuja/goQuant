#include "core/order_book.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace GoQuant
{

    OrderBook::OrderBook(const std::string &symbol) : symbol_(symbol) {}

    bool OrderBook::add_order(Order &order, TradeCallback trade_cb)
    {
        std::lock_guard<std::mutex> lock(book_mutex_);

        if (order.quantity <= 0)
        {
            std::cout << "Rejected order " << order.order_id << ": Invalid quantity" << std::endl;
            return false;
        }

        if (order.type == OrderType::LIMIT && order.price <= 0)
        {
            std::cout << "Rejected order " << order.order_id << ": Invalid price for limit order" << std::endl;
            return false;
        }

        std::cout << "Processing order: " << order.order_id
                  << " " << (order.side == OrderSide::BUY ? "BUY" : "SELL")
                  << " " << order.quantity << " @ "
                  << (order.type == OrderType::MARKET ? "MARKET" : std::to_string(order.price))
                  << std::endl;

        order.status = OrderStatus::ACTIVE;
        match_order(order, trade_cb);

        if (!order.is_fully_filled() && order.type == OrderType::LIMIT)
        {
            add_to_book(order);
            std::cout << "Order resting in book: " << order.order_id
                      << " Leaves: " << order.leaves_quantity << std::endl;
        }
        else if (order.is_fully_filled())
        {
            std::cout << "Order fully filled: " << order.order_id << std::endl;
        }
        else
        {
            std::cout << "Order cancelled/expired: " << order.order_id << std::endl;
        }

        return true;
    }

    void OrderBook::match_order(Order &order, TradeCallback trade_cb)
    {
        switch (order.type)
        {
        case OrderType::MARKET:
            try_match_market_order(order, trade_cb);
            break;
        case OrderType::LIMIT:
            try_match_limit_order(order, trade_cb);
            break;
        case OrderType::IOC:
            try_match_ioc_order(order, trade_cb);
            break;
        case OrderType::FOK:
            try_match_fok_order(order, trade_cb);
            break;
        }
    }

    bool OrderBook::try_match_market_order(Order &order, TradeCallback trade_cb)
    {
        auto &opposite_book = (order.side == OrderSide::BUY) ? asks_ : bids_;

        while (!opposite_book.empty() && !order.is_fully_filled())
        {
            auto best_level = opposite_book.begin();
            double best_price = best_level->first;
            auto &orders_at_price = best_level->second;

            while (!orders_at_price.empty() && !order.is_fully_filled())
            {
                Order &maker_order = orders_at_price.front();

                double fill_quantity = std::min(order.leaves_quantity, maker_order.leaves_quantity);
                double fill_price = maker_order.price;

                execute_trade(order, maker_order, fill_price, fill_quantity, trade_cb);

                if (maker_order.is_fully_filled())
                {
                    remove_from_book(maker_order.order_id);
                    orders_at_price.pop_front();
                }

                if (orders_at_price.empty())
                {
                    opposite_book.erase(best_level);
                    break;
                }
            }
        }

        return order.is_fully_filled();
    }

    bool OrderBook::try_match_limit_order(Order &order, TradeCallback trade_cb)
    {
        auto &opposite_book = (order.side == OrderSide::BUY) ? asks_ : bids_;

        while (!opposite_book.empty() && !order.is_fully_filled())
        {
            auto best_level = opposite_book.begin();
            double best_price = best_level->first;

            bool can_match = (order.side == OrderSide::BUY && order.price >= best_price) ||
                             (order.side == OrderSide::SELL && order.price <= best_price);

            if (!can_match)
                break;

            auto &orders_at_price = best_level->second;

            while (!orders_at_price.empty() && !order.is_fully_filled())
            {
                Order &maker_order = orders_at_price.front();

                double fill_quantity = std::min(order.leaves_quantity, maker_order.leaves_quantity);
                double fill_price = maker_order.price;

                execute_trade(order, maker_order, fill_price, fill_quantity, trade_cb);

                if (maker_order.is_fully_filled())
                {
                    remove_from_book(maker_order.order_id);
                    orders_at_price.pop_front();
                }

                if (orders_at_price.empty())
                {
                    opposite_book.erase(best_level);
                    break;
                }
            }
        }

        return !order.is_fully_filled();
    }

    bool OrderBook::try_match_ioc_order(Order &order, TradeCallback trade_cb)
    {
        try_match_limit_order(order, trade_cb);
        return false;
    }

    bool OrderBook::try_match_fok_order(Order &order, TradeCallback trade_cb)
    {
        auto &opposite_book = (order.side == OrderSide::BUY) ? asks_ : bids_;
        double total_available = 0.0;

        for (const auto &[price, orders] : opposite_book)
        {
            if ((order.side == OrderSide::BUY && price > order.price) ||
                (order.side == OrderSide::SELL && price < order.price))
            {
                break;
            }

            for (const auto &o : orders)
            {
                total_available += o.leaves_quantity;
                if (total_available >= order.quantity)
                {
                    return try_match_limit_order(order, trade_cb);
                }
            }
        }

        order.status = OrderStatus::REJECTED;
        return false;
    }

    void OrderBook::execute_trade(Order &taker, Order &maker, double price,
                                  double quantity, TradeCallback trade_cb)
    {
        taker.fill(quantity, price);
        maker.fill(quantity, price);

        bool is_buyer_maker = (maker.side == OrderSide::BUY);
        Trade trade(symbol_, maker.order_id, taker.order_id, price, quantity,
                    std::chrono::system_clock::now().time_since_epoch().count(),
                    is_buyer_maker);

        std::cout << "TRADE: " << symbol_ << " " << quantity << " @ " << price
                  << " (Maker: " << maker.order_id << ", Taker: " << taker.order_id << ")" << std::endl;

        if (trade_cb)
        {
            trade_cb(trade);
        }
    }

    void OrderBook::add_to_book(Order &order)
    {
        auto &book = (order.side == OrderSide::BUY) ? bids_ : asks_;
        auto &level = book[order.price];

        level.push_back(order);

        OrderLocation loc;
        loc.is_bid = (order.side == OrderSide::BUY);
        loc.price_it = book.find(order.price);
        loc.order_index = level.size() - 1;
        order_lookup_[order.order_id] = loc;
    }

    void OrderBook::remove_from_book(const std::string &order_id)
    {
        auto it = order_lookup_.find(order_id);
        if (it == order_lookup_.end())
            return;

        auto &loc = it->second;
        auto &orders = loc.price_it->second;

        if (loc.order_index < orders.size())
        {
            orders.erase(orders.begin() + loc.order_index);

            for (size_t i = loc.order_index; i < orders.size(); ++i)
            {
                order_lookup_[orders[i].order_id].order_index = i;
            }
        }

        if (orders.empty())
        {
            if (loc.is_bid)
            {
                bids_.erase(loc.price_it);
            }
            else
            {
                asks_.erase(loc.price_it);
            }
        }

        order_lookup_.erase(order_id);
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

    bool OrderBook::modify_order(const std::string &order_id, double new_quantity)
    {
        std::lock_guard<std::mutex> lock(book_mutex_);

        auto it = order_lookup_.find(order_id);
        if (it == order_lookup_.end())
        {
            return false;
        }

        auto &loc = it->second;
        auto &orders = loc.price_it->second;
        Order &order = orders[loc.order_index];

        if (new_quantity < order.filled_quantity)
        {
            return false;
        }

        order.quantity = new_quantity;
        order.leaves_quantity = new_quantity - order.filled_quantity;

        return true;
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
                total_qty += order.leaves_quantity;
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
                total_qty += order.leaves_quantity;
            }
            levels.emplace_back(price, total_qty);
        }

        return levels;
    }

}