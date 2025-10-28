#include "core/advanced_orders.hpp"
#include "utils/uuid_generator.hpp"
#include <iostream>

namespace GoQuant
{

    AdvancedOrderManager::AdvancedOrderManager() {}

    void AdvancedOrderManager::add_stop_loss(const std::string &symbol, OrderSide side,
                                             double quantity, double trigger_price,
                                             double execution_price)
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);

        std::string order_id = generate_order_id();
        OrderType ord_type = (execution_price > 0) ? OrderType::LIMIT : OrderType::MARKET;
        double price = (execution_price > 0) ? execution_price : 0.0;

        AdvancedOrder order(order_id, symbol, AdvancedOrderType::STOP_LOSS,
                            ord_type, side, quantity, price, trigger_price);

        advanced_orders_[symbol].push_back(order);

        std::cout << "Added STOP_LOSS: " << symbol << " " << quantity
                  << " @ trigger " << trigger_price << std::endl;
    }

    void AdvancedOrderManager::add_stop_limit(const std::string &symbol, OrderSide side,
                                              double quantity, double trigger_price,
                                              double limit_price)
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);

        std::string order_id = generate_order_id();
        AdvancedOrder order(order_id, symbol, AdvancedOrderType::STOP_LIMIT,
                            OrderType::LIMIT, side, quantity, limit_price, trigger_price);

        advanced_orders_[symbol].push_back(order);

        std::cout << "Added STOP_LIMIT: " << symbol << " " << quantity
                  << " @ trigger " << trigger_price << " limit " << limit_price << std::endl;
    }

    void AdvancedOrderManager::add_take_profit(const std::string &symbol, OrderSide side,
                                               double quantity, double trigger_price,
                                               double execution_price)
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);

        std::string order_id = generate_order_id();
        OrderType ord_type = (execution_price > 0) ? OrderType::LIMIT : OrderType::MARKET;
        double price = (execution_price > 0) ? execution_price : 0.0;

        AdvancedOrder order(order_id, symbol, AdvancedOrderType::TAKE_PROFIT,
                            ord_type, side, quantity, price, trigger_price);

        advanced_orders_[symbol].push_back(order);

        std::cout << "Added TAKE_PROFIT: " << symbol << " " << quantity
                  << " @ trigger " << trigger_price << std::endl;
    }

    void AdvancedOrderManager::add_trailing_stop(const std::string &symbol, OrderSide side,
                                                 double quantity, double trailing_distance,
                                                 double initial_price)
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);

        std::string order_id = generate_order_id();
        AdvancedOrder order(order_id, symbol, AdvancedOrderType::TRAILING_STOP,
                            OrderType::MARKET, side, quantity, 0.0, initial_price, trailing_distance);

        advanced_orders_[symbol].push_back(order);

        std::cout << "Added TRAILING_STOP: " << symbol << " " << quantity
                  << " @ trail " << trailing_distance << std::endl;
    }

    void AdvancedOrderManager::check_triggers(const std::string &symbol, double current_price)
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);

        auto it = advanced_orders_.find(symbol);
        if (it == advanced_orders_.end())
            return;

        auto &orders = it->second;
        for (auto order_it = orders.begin(); order_it != orders.end();)
        {
            AdvancedOrder &order = *order_it;

            if (order.triggered)
            {
                ++order_it;
                continue;
            }

            bool should_trigger = false;

            switch (order.advanced_type)
            {
            case AdvancedOrderType::STOP_LOSS:
                if (order.side == OrderSide::BUY)
                {
                    should_trigger = current_price >= order.trigger_price;
                }
                else
                {
                    should_trigger = current_price <= order.trigger_price;
                }
                break;

            case AdvancedOrderType::STOP_LIMIT:
                if (order.side == OrderSide::BUY)
                {
                    should_trigger = current_price >= order.trigger_price;
                }
                else
                {
                    should_trigger = current_price <= order.trigger_price;
                }
                break;

            case AdvancedOrderType::TAKE_PROFIT:
                if (order.side == OrderSide::BUY)
                {
                    should_trigger = current_price <= order.trigger_price;
                }
                else
                {
                    should_trigger = current_price >= order.trigger_price;
                }
                break;

            case AdvancedOrderType::TRAILING_STOP:
                if (order.side == OrderSide::BUY)
                {
                    double new_trigger = current_price - order.trailing_distance;
                    if (new_trigger > order.trigger_price)
                    {
                        order.trigger_price = new_trigger;
                    }
                    should_trigger = current_price <= order.trigger_price;
                }
                else
                {
                    double new_trigger = current_price + order.trailing_distance;
                    if (new_trigger < order.trigger_price)
                    {
                        order.trigger_price = new_trigger;
                    }
                    should_trigger = current_price >= order.trigger_price;
                }
                break;
            }

            if (should_trigger)
            {
                trigger_order(order, current_price);
                order_it = orders.erase(order_it);
            }
            else
            {
                ++order_it;
            }
        }
    }

    void AdvancedOrderManager::cancel_advanced_order(const std::string &order_id)
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);

        for (auto &[symbol, orders] : advanced_orders_)
        {
            for (auto it = orders.begin(); it != orders.end(); ++it)
            {
                if (it->order_id == order_id)
                {
                    orders.erase(it);
                    std::cout << "Cancelled advanced order: " << order_id << std::endl;
                    return;
                }
            }
        }
    }

    void AdvancedOrderManager::trigger_order(const AdvancedOrder &advanced_order, double current_price)
    {
        if (!order_callback_)
            return;

        uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();

        Order order(advanced_order.order_id, advanced_order.symbol,
                    advanced_order.order_type, advanced_order.side,
                    advanced_order.quantity, advanced_order.price, timestamp);

        std::cout << "Advanced order triggered: " << advanced_order.order_id
                  << " @ " << current_price << std::endl;

        order_callback_(order);
    }

    std::string AdvancedOrderManager::generate_order_id()
    {
        return "adv_" + UUIDGenerator::generate().substr(0, 8);
    }

}