#ifndef JSON_SERIALIZER_HPP
#define JSON_SERIALIZER_HPP

#include "message_types.hpp"
#include "core/order_types.hpp"
#include "core/trade.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace GoQuant
{

    class JsonSerializer
    {
    public:
        static OrderRequest parse_order_request(const std::string &json_str);
        static CancelRequest parse_cancel_request(const std::string &json_str);
        static MarketDataRequest parse_market_data_request(const std::string &json_str);
        static std::string serialize_order_response(const OrderResponse &response);
        static std::string serialize_error_response(const ErrorResponse &response);
        static std::string serialize_trade(const Trade &trade);
        static std::string serialize_order_book_update(const std::string &symbol,
                                                       const std::vector<std::pair<double, double>> &bids,
                                                       const std::vector<std::pair<double, double>> &asks,
                                                       uint64_t timestamp);
        static std::string serialize_bbo_update(const std::string &symbol,
                                                double best_bid, double best_ask,
                                                uint64_t timestamp);

        static uint64_t get_current_timestamp();
    };

}

#endif