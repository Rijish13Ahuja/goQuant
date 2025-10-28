#ifndef ADVANCED_ORDERS_HPP
#define ADVANCED_ORDERS_HPP

#include "order_types.hpp"
#include <functional>
#include <memory>

namespace GoQuant
{

    enum class AdvancedOrderType
    {
        STOP_LOSS = 0,
        STOP_LIMIT = 1,
        TAKE_PROFIT = 2,
        TRAILING_STOP = 3
    };

    struct AdvancedOrder
    {
        std::string order_id;
        std::string symbol;
        AdvancedOrderType advanced_type;
        OrderType order_type;
        OrderSide side;
        double quantity;
        double price;
        double trigger_price;
        double trailing_distance;
        bool triggered;
        uint64_t timestamp;

        std::function<void(const AdvancedOrder &)> on_trigger;

        AdvancedOrder(const std::string &id, const std::string &sym,
                      AdvancedOrderType adv_type, OrderType ord_type, OrderSide s,
                      double qty, double prc, double trigger_prc, double trail_dist = 0.0)
            : order_id(id), symbol(sym), advanced_type(adv_type), order_type(ord_type),
              side(s), quantity(qty), price(prc), trigger_price(trigger_prc),
              trailing_distance(trail_dist), triggered(false), timestamp(0) {}
    };

    class AdvancedOrderManager
    {
    public:
        AdvancedOrderManager();

        void add_stop_loss(const std::string &symbol, OrderSide side, double quantity,
                           double trigger_price, double execution_price = 0.0);
        void add_stop_limit(const std::string &symbol, OrderSide side, double quantity,
                            double trigger_price, double limit_price);
        void add_take_profit(const std::string &symbol, OrderSide side, double quantity,
                             double trigger_price, double execution_price = 0.0);
        void add_trailing_stop(const std::string &symbol, OrderSide side, double quantity,
                               double trailing_distance, double initial_price = 0.0);

        void check_triggers(const std::string &symbol, double current_price);
        void cancel_advanced_order(const std::string &order_id);

        void set_order_callback(std::function<void(const Order &)> callback)
        {
            order_callback_ = callback;
        }

    private:
        std::unordered_map<std::string, std::vector<AdvancedOrder>> advanced_orders_;
        std::mutex orders_mutex_;
        std::function<void(const Order &)> order_callback_;

        void trigger_order(const AdvancedOrder &advanced_order, double current_price);
        std::string generate_order_id();
    };

}

#endif