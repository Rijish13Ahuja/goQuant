#ifndef TRADE_HPP
#define TRADE_HPP

#include <string>
#include <cstdint>

namespace GoQuant
{

    struct Trade
    {
        std::string trade_id;
        std::string symbol;
        std::string maker_order_id;
        std::string taker_order_id;
        double price;
        double quantity;
        uint64_t timestamp;
        bool is_buyer_maker;

        Trade(const std::string &symbol, const std::string &maker_id,
              const std::string &taker_id, double price, double qty,
              uint64_t ts, bool is_buy_maker)
            : symbol(symbol), maker_order_id(maker_id), taker_order_id(taker_id),
              price(price), quantity(qty), timestamp(ts), is_buyer_maker(is_buy_maker)
        {
            trade_id = "TRADE_" + std::to_string(ts) + "_" + std::to_string(rand() % 10000);
        }
    };

}

#endif