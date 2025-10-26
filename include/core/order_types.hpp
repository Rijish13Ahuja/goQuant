#ifndef ORDER_TYPES_HPP
#define ORDER_TYPES_HPP

#include <string>
#include <cstdint>
#include <functional>

namespace GoQuant
{

    enum class OrderSide : uint8_t
    {
        BUY = 0,
        SELL = 1
    };

    enum class OrderType : uint8_t
    {
        MARKET = 0,
        LIMIT = 1,
        IOC = 2, // Immediate-Or-Cancel
        FOK = 3  // Fill-Or-Kill
    };

    enum class OrderStatus : uint8_t
    {
        PENDING = 0,
        ACTIVE = 1,
        FILLED = 2,
        PARTIALLY_FILLED = 3,
        CANCELLED = 4,
        REJECTED = 5,
        EXPIRED = 6
    };

    struct Order
    {
        std::string order_id;
        std::string symbol;
        OrderType type;
        OrderSide side;
        double quantity;
        double filled_quantity;
        double price;
        uint64_t timestamp;
        OrderStatus status;
        double leaves_quantity;

        Order() = default;

        Order(const std::string &id, const std::string &sym, OrderType t,
              OrderSide s, double qty, double prc, uint64_t ts)
            : order_id(id), symbol(sym), type(t), side(s),
              quantity(qty), filled_quantity(0.0), price(prc),
              timestamp(ts), status(OrderStatus::PENDING),
              leaves_quantity(qty) {}

        bool is_fully_filled() const
        {
            return std::abs(leaves_quantity) < 1e-10;
        }

        bool can_fill(double fill_qty) const
        {
            return fill_qty <= leaves_quantity && status == OrderStatus::ACTIVE;
        }

        void fill(double fill_qty, double fill_price)
        {
            filled_quantity += fill_qty;
            leaves_quantity -= fill_qty;
            if (is_fully_filled())
            {
                status = OrderStatus::FILLED;
            }
            else
            {
                status = OrderStatus::PARTIALLY_FILLED;
            }
        }
    };

    using TradeCallback = std::function<void(const Trade &)>;
    using OrderUpdateCallback = std::function<void(const Order &)>;

}

#endif