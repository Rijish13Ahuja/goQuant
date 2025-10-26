#include <gtest/gtest.h>
#include "../include/core/matching_engine.hpp"

using namespace GoQuant;

class MatchingEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        engine = std::make_unique<MatchingEngine>();
    }

    std::unique_ptr<MatchingEngine> engine;
};

TEST_F(MatchingEngineTest, BasicOrderSubmission)
{
    Order order("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);

    EXPECT_TRUE(engine->submit_order(order));

    auto book = engine->get_order_book("BTC-USDT");
    EXPECT_NE(book, nullptr);
    EXPECT_DOUBLE_EQ(book->get_best_bid(), 50000.0);
}

TEST_F(MatchingEngineTest, CrossSymbolIsolation)
{
    Order btc_order("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);
    Order eth_order("2", "ETH-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 3000.0, 1234567891);

    engine->submit_order(btc_order);
    engine->submit_order(eth_order);

    auto btc_book = engine->get_order_book("BTC-USDT");
    auto eth_book = engine->get_order_book("ETH-USDT");

    EXPECT_DOUBLE_EQ(btc_book->get_best_bid(), 50000.0);
    EXPECT_DOUBLE_EQ(eth_book->get_best_bid(), 3000.0);
}

TEST_F(MatchingEngineTest, OrderCancellation)
{
    Order order("1", "BTC-USDT", OrderType::LIMIT, OrderSide::BUY, 1.0, 50000.0, 1234567890);
    engine->submit_order(order);

    EXPECT_TRUE(engine->cancel_order("BTC-USDT", "1"));
    EXPECT_FALSE(engine->cancel_order("BTC-USDT", "nonexistent"));
}