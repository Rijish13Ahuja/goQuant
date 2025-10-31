#ifndef HEALTH_CHECK_HPP
#define HEALTH_CHECK_HPP

#include "core/matching_engine.hpp"
#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>

namespace GoQuant {

struct HealthStatus {
    bool is_healthy;
    std::string message;
    uint64_t timestamp;
    std::unordered_map<std::string, std::string> details;
    
    HealthStatus() : is_healthy(false), timestamp(0) {}
};

class HealthChecker {
public:
    HealthChecker(MatchingEngine& engine);
    
    HealthStatus check_health();
    void start_continuous_check(int interval_seconds = 30);
    void stop_continuous_check();
    
    bool is_system_healthy() const { return last_status_.is_healthy; }
    HealthStatus get_last_status() const { return last_status_; }

private:
    MatchingEngine& engine_;
    std::atomic<bool> running_{false};
    std::thread health_thread_;
    HealthStatus last_status_;
    
    void run_health_check(int interval_seconds);
    HealthStatus perform_health_check();
};

} 
#endif