#include <iostream>
#include <memory>
#include <iomanip>
#include "core/matching_engine.hpp"
#include "core/order_types.hpp"

using namespace GoQuant;

void print_order_book(const std::shared_ptr<OrderBook> &book)
{
    if (!book)
        return;

    std::cout << "\n📊 " << book->get_symbol() << " Order Book:" << std::endl;
    std::cout << "Best Bid: " << std::fixed << std::setprecision(2) << book->get_best_bid() << std::endl;
    std::cout << "Best Ask: " << std::fixed << std::setprecision(2) << book->get_best_ask() << std::endl;

    auto bids = book->get_bid_levels(5);
    auto asks = book->get_ask_levels(5);

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

int main()
{
    std::cout << "🚀 GoQuant Matching Engine - Day 2: Core Matching Logic" << std::endl;
    std::cout << "======================================================" << std::endl;

    auto engine = std::make_unique<MatchingEngine>();

    std::cout << "\n Testing Matching Engine Scenarios..." << std::endl;

    std::cout << "\n1. Basic Limit Order Matching:" << std::endl;
    Order buy_order("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);
    Order sell_order("2", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 1.0, 50000.0, 1234567891);

    engine->submit_order(buy_order);
    engine->submit_order(sell_order);

    print_order_book(engine->get_order_book("BTC-USDT"));

    std::cout << "\n2. Market Order Execution:" << std::endl;
    Order resting_sell("3", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 2.0, 51000.0, 1234567892);
    Order market_buy("4", "BTC-USDT", OrderType::MARKET, OrderSide::BUY, 1.0, 0.0, 1234567893);

    engine->submit_order(resting_sell);
    engine->submit_order(market_buy);

    print_order_book(engine->get_order_book("BTC-USDT"));

    std::cout << "\n3. IOC Order Execution:" << std::endl;
    Order ioc_sell("5", "BTC-USDT", OrderType::IOC, OrderSide::SELL, 3.0, 52000.0, 1234567894);
    engine->submit_order(ioc_sell);

    print_order_book(engine->get_order_book("BTC-USDT"));

    std::cout << "\n4. FOK Order Execution:" << std::endl;
    Order fok_buy("6", "BTC-USDT", OrderType::FOK, OrderSide::BUY, 5.0, 51000.0, 1234567895);
    engine->submit_order(fok_buy);

    print_order_book(engine->get_order_book("BTC-USDT"));

    std::cout << "\n Day 2 Implementation Complete!" << std::endl;
    std::cout << "Matching engine now supports:" << std::endl;
    std::cout << "  ✓ Price-Time Priority" << std::endl;
    std::cout << "  ✓ All Order Types (Market, Limit, IOC, FOK)" << std::endl;
    std::cout << "  ✓ Trade Execution Reporting" << std::endl;
    std::cout << "  ✓ Comprehensive Unit Tests" << std::endl;

    return 0;
}