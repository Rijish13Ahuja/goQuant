#ifndef ORDER_TYPES_HPP
#define ORDER_TYPES_HPP

#include <string>
#include <cstdint>

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
        IOC = 2,
        FOK = 3
    };

    enum class OrderStatus : uint8_t
    {
        PENDING = 0,
        ACTIVE = 1,
        FILLED = 2,
        PARTIALLY_FILLED = 3,
        CANCELLED = 4,
        REJECTED = 5
    };

    struct Order
    {
        std::string order_id;
        std::string symbol;
        OrderType type;
        OrderSide side;
        double quantity;
        double price;
        uint64_t timestamp;
        OrderStatus status;

        Order() = default;

        Order(const std::string &id, const std::string &sym, OrderType t,
              OrderSide s, double qty, double prc, uint64_t ts)
            : order_id(id), symbol(sym), type(t), side(s),
              quantity(qty), price(prc), timestamp(ts), status(OrderStatus::PENDING) {}
    };

}

#endif