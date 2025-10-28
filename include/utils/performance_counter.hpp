#ifndef PERFORMANCE_COUNTER_HPP
#define PERFORMANCE_COUNTER_HPP

#include <chrono>
#include <atomic>
#include <cstdint>

namespace GoQuant {

class PerformanceCounter {
public:
    PerformanceCounter();
    
    void start();
    void stop();
    void reset();
    
    uint64_t get_elapsed_ns() const;
    uint64_t get_elapsed_us() const;
    uint64_t get_elapsed_ms() const;
    double get_elapsed_seconds() const;

private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    bool running_;
};

class ThroughputCounter {
public:
    ThroughputCounter();
    
    void increment(uint64_t count = 1);
    void reset();
    
    double get_throughput_per_second() const;
    uint64_t get_total_count() const;
    double get_elapsed_seconds() const;

private:
    std::atomic<uint64_t> count_{0};
    std::chrono::high_resolution_clock::time_point start_time_;
};

class LatencyHistogram {
public:
    LatencyHistogram(size_t bucket_count = 100);
    
    void add_latency(uint64_t latency_ns);
    void reset();
    
    void print_histogram() const;
    uint64_t get_percentile(double percentile) const;
    uint64_t get_min_latency() const;
    uint64_t get_max_latency() const;
    double get_average_latency() const;

private:
    std::vector<uint64_t> latencies_;
    mutable std::mutex mutex_;
};

} 

#endif