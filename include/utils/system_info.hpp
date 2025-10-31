#ifndef SYSTEM_INFO_HPP
#define SYSTEM_INFO_HPP

#include <cstdint>

namespace GoQuant {

struct SystemUsage {
    double memory_usage_mb;
    double cpu_percent;
    uint64_t available_memory_mb;
    uint64_t total_memory_mb;
    
    SystemUsage() : memory_usage_mb(0.0), cpu_percent(0.0), available_memory_mb(0), total_memory_mb(0) {}
};

class SystemInfo {
public:
    static SystemUsage get_system_usage();
    static uint64_t get_process_memory_usage();
    static double get_process_cpu_usage();
    static std::string get_hostname();
    static std::string get_os_info();

private:
    static uint64_t last_total_time_;
    static uint64_t last_process_time_;
};

} 

#endif