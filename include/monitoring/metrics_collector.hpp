#ifndef METRICS_COLLECTOR_HPP
#define METRICS_COLLECTOR_HPP

#include "core/matching_engine.hpp"
#include "utils/performance_counter.hpp"
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <memory>
#include <thread>

namespace GoQuant {

class MetricsCollector {
public:
    MetricsCollector(MatchingEngine& engine, int port = 8080);
    ~MetricsCollector();
    
    void start();
    void stop();
    void update_metrics();

private:
    MatchingEngine& engine_;
    int port_;
    std::atomic<bool> running_{false};
    std::thread metrics_thread_;
    
    std::shared_ptr<prometheus::Registry> registry_;
    
    prometheus::Counter& orders_counter_;
    prometheus::Counter& trades_counter_;
    prometheus::Gauge& throughput_gauge_;
    prometheus::Gauge& latency_gauge_;
    prometheus::Gauge& memory_usage_gauge_;
    prometheus::Gauge& cpu_usage_gauge_;
    prometheus::Histogram& order_latency_histogram_;
    
    void run_metrics_server();
    void collect_metrics();
};

} 

#endif