#include <iostream>
#include <memory>
#include <iomanip>
#include <signal.h>
#include "core/matching_engine.hpp"
#include "core/order_types.hpp"
#include "api/websocket_server.hpp"
#include "market_data/market_data_feed.hpp"

using namespace GoQuant;

std::unique_ptr<WebSocketServer> ws_server;
std::unique_ptr<MarketDataFeed> market_data_feed;

void signal_handler(int signal)
{
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (market_data_feed)
    {
        market_data_feed->stop();
    }
    if (ws_server)
    {
        ws_server->stop();
    }
    exit(0);
}

void setup_signal_handlers()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

int main()
{
    setup_signal_handlers();

    std::cout << " GoQuant Matching Engine - Day 3: WebSocket API" << std::endl;
    std::cout << "=================================================" << std::endl;

    auto engine = std::make_unique<MatchingEngine>();

    ws_server = std::make_unique<WebSocketServer>(*engine, 9001);

    market_data_feed = std::make_unique<MarketDataFeed>(*engine, *ws_server);
    engine->set_trade_callback([&](const Trade &trade)
                               { market_data_feed->on_trade_executed(trade); });

    std::cout << "\n Starting Services..." << std::endl;
    ws_server->start();
    market_data_feed->start();

    std::cout << "\n System Ready!" << std::endl;
    std::cout << "WebSocket API: ws://localhost:9001" << std::endl;
    std::cout << "Health Check: http://localhost:9001/health" << std::endl;
    std::cout << "\nAvailable Symbols: BTC-USDT, ETH-USDT" << std::endl;
    std::cout << "\nPress Ctrl+C to stop the server" << std::endl;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}