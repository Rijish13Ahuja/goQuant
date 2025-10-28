#include "api/json_serializer.hpp"
#include <chrono>

namespace GoQuant
{

    using json = nlohmann::json;

    OrderRequest JsonSerializer::parse_order_request(const std::string &json_str)
    {
        auto j = json::parse(json_str);
        OrderRequest request;

        request.symbol = j.value("symbol", "");
        request.order_type = j.value("order_type", "");
        request.side = j.value("side", "");
        request.quantity = j.value("quantity", 0.0);
        request.price = j.value("price", 0.0);
        request.order_id = j.value("order_id", "");

        return request;
    }

    CancelRequest JsonSerializer::parse_cancel_request(const std::string &json_str)
    {
        auto j = json::parse(json_str);
        CancelRequest request;

        request.symbol = j.value("symbol", "");
        request.order_id = j.value("order_id", "");

        return request;
    }

    MarketDataRequest JsonSerializer::parse_market_data_request(const std::string &json_str)
    {
        auto j = json::parse(json_str);
        MarketDataRequest request;

        request.symbol = j.value("symbol", "");
        request.type = j.value("type", "");

        return request;
    }

    std::string JsonSerializer::serialize_order_response(const OrderResponse &response)
    {
        json j;
        j["type"] = "order_response";
        j["order_id"] = response.order_id;
        j["status"] = response.status;
        j["message"] = response.message;
        j["filled_quantity"] = response.filled_quantity;
        j["average_price"] = response.average_price;
        j["timestamp"] = get_current_timestamp();

        return j.dump();
    }

    std::string JsonSerializer::serialize_error_response(const ErrorResponse &response)
    {
        json j;
        j["type"] = "error";
        j["error"] = response.error;
        j["message"] = response.message;
        j["timestamp"] = get_current_timestamp();

        return j.dump();
    }

    std::string JsonSerializer::serialize_trade(const Trade &trade)
    {
        json j;
        j["type"] = "trade";
        j["timestamp"] = trade.timestamp;
        j["symbol"] = trade.symbol;
        j["trade_id"] = trade.trade_id;
        j["price"] = trade.price;
        j["quantity"] = trade.quantity;
        j["aggressor_side"] = trade.aggressor_side;
        j["maker_order_id"] = trade.maker_order_id;
        j["taker_order_id"] = trade.taker_order_id;

        return j.dump();
    }

    std::string JsonSerializer::serialize_order_book_update(const std::string &symbol,
                                                            const std::vector<std::pair<double, double>> &bids,
                                                            const std::vector<std::pair<double, double>> &asks,
                                                            uint64_t timestamp)
    {
        json j;
        j["type"] = "order_book";
        j["timestamp"] = timestamp;
        j["symbol"] = symbol;

        json bids_array = json::array();
        for (const auto &[price, quantity] : bids)
        {
            json level = json::array();
            level.push_back(price);
            level.push_back(quantity);
            bids_array.push_back(level);
        }
        j["bids"] = bids_array;

        // Serialize asks
        json asks_array = json::array();
        for (const auto &[price, quantity] : asks)
        {
            json level = json::array();
            level.push_back(price);
            level.push_back(quantity);
            asks_array.push_back(level);
        }
        j["asks"] = asks_array;

        return j.dump();
    }

    std::string JsonSerializer::serialize_bbo_update(const std::string &symbol,
                                                     double best_bid, double best_ask,
                                                     uint64_t timestamp)
    {
        json j;
        j["type"] = "bbo";
        j["timestamp"] = timestamp;
        j["symbol"] = symbol;
        j["best_bid"] = best_bid;
        j["best_ask"] = best_ask;
        j["spread"] = best_ask - best_bid;

        return j.dump();
    }

    uint64_t JsonSerializer::get_current_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   now.time_since_epoch())
            .count();
    }

}