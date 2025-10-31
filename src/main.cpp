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
#include "config/config_manager.hpp"
#include "monitoring/health_check.hpp"
#include "utils/performance_counter.hpp"
#include "utils/system_info.hpp"

using namespace GoQuant;

std::unique_ptr<WebSocketServer> ws_server;
std::unique_ptr<MarketDataFeed> market_data_feed;
std::unique_ptr<SnapshotManager> snapshot_manager;
std::unique_ptr<HealthChecker> health_checker;
std::unique_ptr<MatchingEngine> engine;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (market_data_feed) {
        market_data_feed->stop();
    }
    if (ws_server) {
        ws_server->stop();
    }
    if (health_checker) {
        health_checker->stop_continuous_check();
    }
    exit(0);
}

void setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void run_performance_monitor() {
    auto& config = ConfigManager::get_instance().get_engine_config();
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(config.performance_stats_interval));
        
        if (engine) {
            double throughput = engine->get_throughput_ops();
            uint64_t orders_processed = engine->get_orders_processed();
            
            auto system_info = SystemInfo::get_system_usage();
            
            std::cout << "\n=== Performance Stats ===" << std::endl;
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) 
                      << throughput << " orders/sec" << std::endl;
            std::cout << "Total Orders: " << orders_processed << std::endl;
            std::cout << "Memory Usage: " << system_info.memory_usage_mb << " MB" << std::endl;
            std::cout << "CPU Usage: " << std::fixed << std::setprecision(1) 
                      << system_info.cpu_percent << "%" << std::endl;
        }
    }
}

void run_snapshot_scheduler() {
    auto& config = ConfigManager::get_instance().get_engine_config();
    
    if (!config.enable_persistence) {
        return;
    }
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(config.snapshot_interval_seconds));
        
        if (engine && snapshot_manager) {
            auto symbol_configs = ConfigManager::get_instance().get_all_symbol_configs();
            
            for (const auto& symbol_config : symbol_configs) {
                auto book = engine->get_order_book(symbol_config.symbol);
                if (book) {
                    auto bids = book->get_bid_levels(config.order_book_depth);
                    auto asks = book->get_ask_levels(config.order_book_depth);
                    snapshot_manager->save_snapshot(symbol_config.symbol, bids, asks);
                }
            }
            
            std::cout << "Order book snapshots saved for " << symbol_configs.size() << " symbols" << std::endl;
            
            snapshot_manager->cleanup_old_snapshots(7);
        }
    }
}

void print_banner() {
    std::cout << R"(
   _____       ___                  _   
  / ____|     / _ \                | |  
 | |  __  ___| | | |_   _  __ _  __| |  
 | | |_ |/ _ \ | | | | | |/ _` |/ _` |  
 | |__| |  __/ |_| | |_| | (_| | (_| |  
  \_____|\___|\___/ \__,_|\__,_|\__,_|  
                                        
    )" << std::endl;
    
    std::cout << " GoQuant Matching Engine - Production Ready" << std::endl;
    std::cout << "============================================" << std::endl;
}

int main(int argc, char* argv[]) {
    setup_signal_handlers();
    print_banner();
    
    std::string config_path = "config/default.json";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    const char* env_config = std::getenv("CONFIG_PATH");
    if (env_config) {
        config_path = env_config;
    }
    
    std::cout << "Loading configuration from: " << config_path << std::endl;
    
    if (!ConfigManager::get_instance().load_config(config_path)) {
        std::cerr << "Failed to load configuration, using defaults" << std::endl;
    }
    
    auto config = ConfigManager::get_instance().get_engine_config();
    
    engine = std::make_unique<MatchingEngine>();
    ws_server = std::make_unique<WebSocketServer>(*engine, config.websocket_port);
    market_data_feed = std::make_unique<MarketDataFeed>(*engine, *ws_server);
    snapshot_manager = std::make_unique<SnapshotManager>(config.persistence_path + "orderbook.db");
    health_checker = std::make_unique<HealthChecker>(*engine);
    
    if (!snapshot_manager->initialize()) {
        std::cerr << "Failed to initialize snapshot manager" << std::endl;
    }
    
    auto symbol_configs = ConfigManager::get_instance().get_all_symbol_configs();
    for (const auto& symbol_config : symbol_configs) {
        engine->add_symbol(symbol_config.symbol);
    }
    
    engine->set_trade_callback([&](const Trade& trade) {
        market_data_feed->on_trade_executed(trade);
    });
    
    engine->get_fee_calculator().set_fee_structure(
        FeeStructure(config.maker_fee, config.taker_fee));
    
    std::cout << "\n Starting Services..." << std::endl;
    ws_server->start();
    market_data_feed->start();
    health_checker->start_continuous_check(30);
    
    std::thread perf_monitor(run_performance_monitor);
    std::thread snapshot_scheduler(run_snapshot_scheduler);
    
    auto system_info = SystemInfo::get_system_usage();
    
    std::cout << "\n System Ready!" << std::endl;
    std::cout << "Host: " << SystemInfo::get_hostname() << std::endl;
    std::cout << "OS: " << SystemInfo::get_os_info() << std::endl;
    std::cout << "Memory: " << system_info.memory_usage_mb << "MB / " 
              << system_info.total_memory_mb << "MB" << std::endl;
    std::cout << "\n📡 Endpoints:" << std::endl;
    std::cout << "WebSocket API: ws://localhost:" << config.websocket_port << std::endl;
    std::cout << "Health Check: http://localhost:" << config.websocket_port << "/health" << std::endl;
    std::cout << "Metrics: http://localhost:" << config.websocket_port << "/metrics" << std::endl;
    
    std::cout << "\n Configuration:" << std::endl;
    std::cout << "Symbols: " << symbol_configs.size() << std::endl;
    std::cout << "Maker Fee: " << (config.maker_fee * 100) << "%" << std::endl;
    std::cout << "Taker Fee: " << (config.taker_fee * 100) << "%" << std::endl;
    std::cout << "Persistence: " << (config.enable_persistence ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Advanced Orders: " << (config.enable_advanced_orders ? "Enabled" : "Disabled") << std::endl;
    
    std::cout << "\n Press Ctrl+C to stop the server" << std::endl;
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        for (const auto& symbol_config : symbol_configs) {
            auto book = engine->get_order_book(symbol_config.symbol);
            if (book) {
                double mid_price = (book->get_best_bid() + book->get_best_ask()) / 2.0;
                if (mid_price > 0) {
                    engine->update_market_price(symbol_config.symbol, mid_price);
                }
            }
        }
        
        auto health_status = health_checker->get_last_status();
        if (!health_status.is_healthy) {
            std::cerr << "Health Check Alert: " << health_status.message << std::endl;
        }
    }
    
    perf_monitor.join();
    snapshot_scheduler.join();
    
    return 0;
}