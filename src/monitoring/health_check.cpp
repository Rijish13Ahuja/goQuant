#include "monitoring/health_check.hpp"
#include "utils/system_info.hpp"
#include <iostream>

namespace GoQuant {

HealthChecker::HealthChecker(MatchingEngine& engine) : engine_(engine) {}

HealthStatus HealthChecker::check_health() {
    return perform_health_check();
}

void HealthChecker::start_continuous_check(int interval_seconds) {
    if (running_) return;
    
    running_ = true;
    health_thread_ = std::thread(&HealthChecker::run_health_check, this, interval_seconds);
}

void HealthChecker::stop_continuous_check() {
    if (!running_) return;
    
    running_ = false;
    if (health_thread_.joinable()) {
        health_thread_.join();
    }
}

void HealthChecker::run_health_check(int interval_seconds) {
    while (running_) {
        last_status_ = perform_health_check();
        
        if (!last_status_.is_healthy) {
            std::cerr << "HEALTH CHECK FAILED: " << last_status_.message << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
}

HealthStatus HealthChecker::perform_health_check() {
    HealthStatus status;
    status.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    try {
        status.details["engine_throughput"] = std::to_string(engine_.get_throughput_ops());
        status.details["total_orders"] = std::to_string(engine_.get_orders_processed());
        
        auto btc_book = engine_.get_order_book("BTC-USDT");
        if (btc_book) {
            double best_bid = btc_book->get_best_bid();
            double best_ask = btc_book->get_best_ask();
            status.details["btc_bbo"] = std::to_string(best_bid) + "/" + std::to_string(best_ask);
        }
        
        auto system_info = SystemInfo::get_system_usage();
        status.details["memory_usage_mb"] = std::to_string(system_info.memory_usage_mb);
        status.details["cpu_percent"] = std::to_string(system_info.cpu_percent);
        
        if (system_info.memory_usage_mb > 1024) {
            status.is_healthy = false;
            status.message = "High memory usage: " + std::to_string(system_info.memory_usage_mb) + "MB";
            return status;
        }
        
        if (system_info.cpu_percent > 90.0) {
            status.is_healthy = false;
            status.message = "High CPU usage: " + std::to_string(system_info.cpu_percent) + "%";
            return status;
        }
        
        status.is_healthy = true;
        status.message = "All systems operational";
        
    } catch (const std::exception& e) {
        status.is_healthy = false;
        status.message = "Health check exception: " + std::string(e.what());
    }
    
    return status;
}

}