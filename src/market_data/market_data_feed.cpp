#include "market_data/market_data_feed.hpp"
#include "api/json_serializer.hpp"
#include <thread>
#include <iostream>
namespace GoQuant
{
    MarketDataFeed::MarketDataFeed(MatchingEngine &engine, WebSocketServer &ws_server)
        : engine_(engine), ws_server_(ws_server), running_(false) {}
    MarketDataFeed::~MarketDataFeed()
    {
        stop();
    }
    void MarketDataFeed::start()
    {
        if (running_)
            return;

        running_ = true;
        feed_thread_ = std::thread(&MarketDataFeed::run_feed, this);
        std::cout << "Market data feed started" << std::endl;
    }
    void MarketDataFeed::stop()
    {
        if (!running_)
            return;

        running_ = false;
        if (feed_thread_.joinable())
        {
            feed_thread_.join();
        }
        std::cout << "Market data feed stopped" << std::endl;
    }
    void MarketDataFeed::run_feed()
    {
        while (running_)
        {
            auto symbols = get_active_symbols();
            for (const auto &symbol : symbols)
            {
                broadcast_bbo_update(symbol);
                broadcast_depth_update(symbol);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    void MarketDataFeed::on_trade_executed(const Trade &trade)
    {
        ws_server_.broadcast_trade(trade);
    }
    void MarketDataFeed::broadcast_bbo_update(const std::string &symbol)
    {
        auto book = engine_.get_order_book(symbol);
        if (!book)
            return;

        double best_bid = book->get_best_bid();
        double best_ask = book->get_best_ask();

        if (best_bid > 0 && best_ask > 0 && best_ask > best_bid)
        {
            auto bbo_msg = JsonSerializer::serialize_bbo_update(
                symbol, best_bid, best_ask, JsonSerializer::get_current_timestamp());
            ws_server_.broadcast_market_data(symbol, bbo_msg);
        }
    }
    void MarketDataFeed::broadcast_depth_update(const std::string &symbol)
    {
        auto book = engine_.get_order_book(symbol);
        if (!book)
            return;

        auto bids = book->get_bid_levels(10);
        auto asks = book->get_ask_levels(10);

        if (!bids.empty() || !asks.empty())
        {
            auto depth_msg = JsonSerializer::serialize_order_book_update(
                symbol, bids, asks, JsonSerializer::get_current_timestamp());
            ws_server_.broadcast_market_data(symbol, depth_msg);
        }
    }
    std::vector<std::string> MarketDataFeed::get_active_symbols()
    {
        return {"BTC-USDT", "ETH-USDT"};
    }
}