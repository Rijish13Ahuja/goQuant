#include <iostream>
#include <memory>
#include <iomanip>
#include <signal.h>
#include <thread>
#include "core/matching_engine.hpp"
#include "core/order_types.hpp"
#include "api/websocket_server.hpp"
#include "market_data/market_data_feed.hpp"
#include "persistence/snapshot_manager.hpp"
#include "utils/performance_counter.hpp"

using namespace GoQuant;

std::unique_ptr<WebSocketServer> ws_server;
std::unique_ptr<MarketDataFeed> market_data_feed;
std::unique_ptr<SnapshotManager> snapshot_manager;
std::unique_ptr<MatchingEngine> engine;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (market_data_feed) {
        market_data_feed->stop();
    }
    if (ws_server) {
        ws_server->stop();
    }
    exit(0);
}

void setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void run_performance_monitor() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (engine) {
            double throughput = engine->get_throughput_ops();
            uint64_t orders_processed = engine->get_orders_processed();
            
            std::cout << "\n=== Performance Stats ===" << std::endl;
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) 
                      << throughput << " orders/sec" << std::endl;
            std::cout << "Total Orders: " << orders_processed << std::endl;
        }
    }
}

void run_snapshot_scheduler() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
        
        if (engine && snapshot_manager) {
            auto btc_book = engine->get_order_book("BTC-USDT");
            if (btc_book) {
                auto bids = btc_book->get_bid_levels(10);
                auto asks = btc_book->get_ask_levels(10);
                snapshot_manager->save_snapshot("BTC-USDT", bids, asks);
            }
            
            auto eth_book = engine->get_order_book("ETH-USDT");
            if (eth_book) {
                auto bids = eth_book->get_bid_levels(10);
                auto asks = eth_book->get_ask_levels(10);
                snapshot_manager->save_snapshot("ETH-USDT", bids, asks);
            }
            
            std::cout << "Order book snapshots saved" << std::endl;
        }
    }
}

int main() {
    setup_signal_handlers();
    
    std::cout << " GoQuant Matching Engine - Day 4: Performance & Advanced Features" << std::endl;
    std::cout << "==================================================================" << std::endl;
    
    engine = std::make_unique<MatchingEngine>();
    ws_server = std::make_unique<WebSocketServer>(*engine, 9001);
    market_data_feed = std::make_unique<MarketDataFeed>(*engine, *ws_server);
    snapshot_manager = std::make_unique<SnapshotManager>();
    
    if (!snapshot_manager->initialize()) {
        std::cerr << "Failed to initialize snapshot manager" << std::endl;
    }
    
    engine->set_trade_callback([&](const Trade& trade) {
        market_data_feed->on_trade_executed(trade);
    });
    
    std::cout << "\n Starting Services..." << std::endl;
    ws_server->start();
    market_data_feed->start();
    
    std::thread perf_monitor(run_performance_monitor);
    std::thread snapshot_scheduler(run_snapshot_scheduler);
    
    std::cout << "\n System Ready!" << std::endl;
    std::cout << "WebSocket API: ws://localhost:9001" << std::endl;
    std::cout << "Health Check: http://localhost:9001/health" << std::endl;
    std::cout << "\nAdvanced Features:" << std::endl;
    std::cout << "  Stop-Loss Orders" << std::endl;
    std::cout << "  Stop-Limit Orders" << std::endl;
    std::cout << "  Take-Profit Orders" << std::endl;
    std::cout << "  Trailing Stop Orders" << std::endl;
    std::cout << "  Performance Monitoring" << std::endl;
    std::cout << "  Order Book Persistence" << std::endl;
    std::cout << "  Fee Calculations" << std::endl;
    std::cout << "\nPress Ctrl+C to stop the server" << std::endl;
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        engine->update_market_price("BTC-USDT", 50000.0);
        engine->update_market_price("ETH-USDT", 3000.0);
    }
    
    perf_monitor.join();
    snapshot_scheduler.join();
    
    return 0;
}