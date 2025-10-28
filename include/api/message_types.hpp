#ifndef MESSAGE_TYPES_HPP
#define MESSAGE_TYPES_HPP
#include <string>
#include <vector>
#include <cstdint>
namespace GoQuant
{
    struct OrderRequest
    {
        std::string symbol;
        std::string order_type;
        std::string side;
        double quantity;
        double price;
        std::string order_id;

        OrderRequest() = default;
        OrderRequest(const std::string &sym, const std::string &type,
                     const std::string &s, double qty, double prc = 0.0,
                     const std::string &id = "")
            : symbol(sym), order_type(type), side(s), quantity(qty), price(prc), order_id(id) {}
    };
    struct CancelRequest
    {
        std::string symbol;
        std::string order_id;

        CancelRequest() = default;
        CancelRequest(const std::string &sym, const std::string &id)
            : symbol(sym), order_id(id) {}
    };
    struct MarketDataRequest
    {
        std::string symbol;
        std::string type;

        MarketDataRequest() = default;
        MarketDataRequest(const std::string &sym, const std::string &t)
            : symbol(sym), type(t) {}
    };
    struct OrderResponse
    {
        std::string order_id;
        std::string status;
        std::string message;
        double filled_quantity;
        double average_price;

        OrderResponse() = default;
        OrderResponse(const std::string &id, const std::string &stat,
                      const std::string &msg = "", double filled = 0.0, double avg_price = 0.0)
            : order_id(id), status(stat), message(msg), filled_quantity(filled), average_price(avg_price) {}
    };
    struct ErrorResponse
    {
        std::string error;
        std::string message;

        ErrorResponse() = default;
        ErrorResponse(const std::string &err, const std::string &msg)
            : error(err), message(msg) {}
    };
}
#endif