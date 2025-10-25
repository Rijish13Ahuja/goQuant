#include <iostream>
#include <memory>
#include "core/matching_engine.hpp"
#include "core/order_types.hpp"

using namespace GoQuant;

int main()
{
    std::cout << "🚀 GoQuant Matching Engine - Day 1 Setup" << std::endl;
    std::cout << "========================================" << std::endl;

    auto engine = std::make_unique<MatchingEngine>();

    std::cout << "\n📈 Testing Basic Order Book Operations..." << std::endl;

    Order buy_order1("order1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);
    Order sell_order1("order2", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 1.0, 51000.0, 1234567891);

    std::cout << "\n➡️  Submitting orders..." << std::endl;
    engine->submit_order(buy_order1);
    engine->submit_order(sell_order1);

    auto btc_book = engine->get_order_book("BTC-USDT");
    if (btc_book)
    {
        std::cout << "\n📊 Order Book State:" << std::endl;
        std::cout << "Best Bid: " << btc_book->get_best_bid() << std::endl;
        std::cout << "Best Ask: " << btc_book->get_best_ask() << std::endl;

        auto bids = btc_book->get_bid_levels(5);
        auto asks = btc_book->get_ask_levels(5);

        std::cout << "\nBid Levels:" << std::endl;
        for (const auto &[price, qty] : bids)
        {
            std::cout << "  " << price << " : " << qty << std::endl;
        }

        std::cout << "\nAsk Levels:" << std::endl;
        for (const auto &[price, qty] : asks)
        {
            std::cout << "  " << price << " : " << qty << std::endl;
        }
    }

    std::cout << "\n❌ Testing order cancellation..." << std::endl;
    engine->cancel_order("BTC-USDT", "order1");

    if (btc_book)
    {
        std::cout << "\n📊 Order Book After Cancellation:" << std::endl;
        std::cout << "Best Bid: " << btc_book->get_best_bid() << std::endl;
        std::cout << "Best Ask: " << btc_book->get_best_ask() << std::endl;
    }

    std::cout << "\n✅ Day 1 Setup Complete!" << std::endl;
    std::cout << "Tomorrow we'll implement the actual matching logic." << std::endl;

    return 0;
}