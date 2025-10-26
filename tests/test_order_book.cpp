#include <gtest/gtest.h>
#include "../include/core/order_book.hpp"
#include "../include/core/order_types.hpp"

using namespace GoQuant;

class OrderBookTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        book = std::make_unique<OrderBook>("BTC-USDT");
    }

    std::unique_ptr<OrderBook> book;
};

TEST_F(OrderBookTest, AddLimitOrder)
{
    Order order("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);
    std::vector<Trade> trades;

    EXPECT_TRUE(book->add_order(order, trades));
    EXPECT_EQ(order.status, OrderStatus::ACTIVE);
    EXPECT_TRUE(trades.empty());
    EXPECT_DOUBLE_EQ(book->get_best_bid(), 50000.0);
}

TEST_F(OrderBookTest, BasicMatching)
{
    Order buy("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);
    std::vector<Trade> trades1;
    book->add_order(buy, trades1);

    Order sell("2", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 1.0, 50000.0, 1234567891);
    std::vector<Trade> trades2;
    book->add_order(sell, trades2);

    EXPECT_EQ(trades2.size(), 1);
    EXPECT_DOUBLE_EQ(trades2[0].price, 50000.0);
    EXPECT_DOUBLE_EQ(trades2[0].quantity, 1.0);
    EXPECT_EQ(sell.status, OrderStatus::FILLED);
}

TEST_F(OrderBookTest, PriceTimePriority)
{
    Order buy1("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1000);
    Order buy2("2", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1001);
    std::vector<Trade> trades1, trades2;
    book->add_order(buy1, trades1);
    book->add_order(buy2, trades2);

    Order sell("3", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 1.0, 50000.0, 1002);
    std::vector<Trade> trades3;
    book->add_order(sell, trades3);

    EXPECT_EQ(trades3.size(), 1);
    EXPECT_EQ(trades3[0].maker_order_id, "1");
    EXPECT_EQ(buy1.status, OrderStatus::FILLED);
    EXPECT_EQ(buy2.status, OrderStatus::ACTIVE);
}

TEST_F(OrderBookTest, MarketOrder)
{
    Order sell("1", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 1.0, 50000.0, 1234567890);
    std::vector<Trade> trades1;
    book->add_order(sell, trades1);

    Order market_buy("2", "BTC-USDT", OrderType::MARKET, OrderSide::BUY, 1.0, 0.0, 1234567891);
    std::vector<Trade> trades2;
    book->add_order(market_buy, trades2);

    EXPECT_EQ(trades2.size(), 1);
    EXPECT_DOUBLE_EQ(trades2[0].price, 50000.0);
    EXPECT_EQ(market_buy.status, OrderStatus::FILLED);
}

TEST_F(OrderBookTest, IOCOrder)
{
    Order sell("1", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 0.5, 50000.0, 1234567890);
    std::vector<Trade> trades1;
    book->add_order(sell, trades1);

    Order ioc_buy("2", "BTC-USDT", OrderType::IOC, OrderSide::BUY, 1.0, 50000.0, 1234567891);
    std::vector<Trade> trades2;
    book->add_order(ioc_buy, trades2);

    EXPECT_EQ(trades2.size(), 1);
    EXPECT_DOUBLE_EQ(trades2[0].quantity, 0.5);
    EXPECT_EQ(ioc_buy.status, OrderStatus::PARTIALLY_FILLED);
}

TEST_F(OrderBookTest, FOKOrder)
{
    Order sell("1", "BTC-USDT", OrderType::LIMIT, OrderSide::SELL, 0.5, 50000.0, 1234567890);
    std::vector<Trade> trades1;
    book->add_order(sell, trades1);

    Order fok_buy("2", "BTC-USDT", OrderType::FOK, OrderSide::BUY, 1.0, 50000.0, 1234567891);
    std::vector<Trade> trades2;
    book->add_order(fok_buy, trades2);

    EXPECT_TRUE(trades2.empty());
    EXPECT_EQ(fok_buy.status, OrderStatus::CANCELLED);
}