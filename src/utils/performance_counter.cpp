#include "utils/performance_counter.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace GoQuant {

PerformanceCounter::PerformanceCounter() : running_(false) {}

void PerformanceCounter::start() {
    start_time_ = std::chrono::high_resolution_clock::now();
    running_ = true;
}

void PerformanceCounter::stop() {
    if (running_) {
        end_time_ = std::chrono::high_resolution_clock::now();
        running_ = false;
    }
}

void PerformanceCounter::reset() {
    running_ = false;
}

uint64_t PerformanceCounter::get_elapsed_ns() const {
    auto end = running_ ? std::chrono::high_resolution_clock::now() : end_time_;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_time_).count();
}

uint64_t PerformanceCounter::get_elapsed_us() const {
    return get_elapsed_ns() / 1000;
}

uint64_t PerformanceCounter::get_elapsed_ms() const {
    return get_elapsed_ns() / 1000000;
}

double PerformanceCounter::get_elapsed_seconds() const {
    return static_cast<double>(get_elapsed_ns()) / 1e9;
}

ThroughputCounter::ThroughputCounter() {
    start_time_ = std::chrono::high_resolution_clock::now();
}

void ThroughputCounter::increment(uint64_t count) {
    count_ += count;
}

void ThroughputCounter::reset() {
    count_ = 0;
    start_time_ = std::chrono::high_resolution_clock::now();
}

double ThroughputCounter::get_throughput_per_second() const {
    double elapsed = get_elapsed_seconds();
    return elapsed > 0 ? static_cast<double>(count_) / elapsed : 0.0;
}

uint64_t ThroughputCounter::get_total_count() const {
    return count_;
}

double ThroughputCounter::get_elapsed_seconds() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time_).count();
}

LatencyHistogram::LatencyHistogram(size_t bucket_count) {
    latencies_.reserve(bucket_count * 1000);
}

void LatencyHistogram::add_latency(uint64_t latency_ns) {
    std::lock_guard<std::mutex> lock(mutex_);
    latencies_.push_back(latency_ns);
}

void LatencyHistogram::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    latencies_.clear();
}

void LatencyHistogram::print_histogram() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latencies_.empty()) return;
    
    auto sorted = latencies_;
    std::sort(sorted.begin(), sorted.end());
    
    uint64_t min = sorted.front();
    uint64_t max = sorted.back();
    uint64_t p50 = sorted[sorted.size() * 0.5];
    uint64_t p90 = sorted[sorted.size() * 0.9];
    uint64_t p95 = sorted[sorted.size() * 0.95];
    uint64_t p99 = sorted[sorted.size() * 0.99];
    uint64_t p999 = sorted[sorted.size() * 0.999];
    
    double avg = get_average_latency();
    
    std::cout << "\n=== Latency Histogram (ns) ===" << std::endl;
    std::cout << "Count: " << sorted.size() << std::endl;
    std::cout << "Min: " << min << std::endl;
    std::cout << "Max: " << max << std::endl;
    std::cout << "Avg: " << static_cast<uint64_t>(avg) << std::endl;
    std::cout << "P50: " << p50 << std::endl;
    std::cout << "P90: " << p90 << std::endl;
    std::cout << "P95: " << p95 << std::endl;
    std::cout << "P99: " << p99 << std::endl;
    std::cout << "P99.9: " << p999 << std::endl;
}

uint64_t LatencyHistogram::get_percentile(double percentile) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latencies_.empty()) return 0;
    
    auto sorted = latencies_;
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(sorted.size() * percentile);
    return sorted[std::min(index, sorted.size() - 1)];
}

uint64_t LatencyHistogram::get_min_latency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latencies_.empty()) return 0;
    return *std::min_element(latencies_.begin(), latencies_.end());
}

uint64_t LatencyHistogram::get_max_latency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latencies_.empty()) return 0;
    return *std::max_element(latencies_.begin(), latencies_.end());
}

double LatencyHistogram::get_average_latency() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latencies_.empty()) return 0.0;
    
    double sum = 0.0;
    for (auto latency : latencies_) {
        sum += static_cast<double>(latency);
    }
    return sum / latencies_.size();
}

} 