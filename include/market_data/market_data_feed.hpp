#ifndef MARKET_DATA_FEED_HPP
#define MARKET_DATA_FEED_HPP

#include "core/matching_engine.hpp"
#include "api/websocket_server.hpp"
#include "core/trade.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <string>

namespace GoQuant
{

    class MarketDataFeed
    {
    public:
        MarketDataFeed(MatchingEngine &engine, WebSocketServer &ws_server);
        ~MarketDataFeed();

        void start();
        void stop();
        void on_trade_executed(const Trade &trade);

    private:
        MatchingEngine &engine_;
        WebSocketServer &ws_server_;
        std::atomic<bool> running_{false};
        std::thread feed_thread_;

        void run_feed();
        void broadcast_bbo_update(const std::string &symbol);
        void broadcast_depth_update(const std::string &symbol);
        std::vector<std::string> get_active_symbols();
    };

}

#endif