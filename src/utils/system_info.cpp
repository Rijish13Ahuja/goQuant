#include "utils/system_info.hpp"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

namespace GoQuant {

uint64_t SystemInfo::last_total_time_ = 0;
uint64_t SystemInfo::last_process_time_ = 0;

SystemUsage SystemInfo::get_system_usage() {
    SystemUsage usage;
    
#ifdef __linux__
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        usage.total_memory_mb = (sys_info.totalram * sys_info.mem_unit) / (1024 * 1024);
        usage.available_memory_mb = (sys_info.freeram * sys_info.mem_unit) / (1024 * 1024);
        usage.memory_usage_mb = static_cast<double>(get_process_memory_usage()) / 1024.0;
    }
    
    usage.cpu_percent = get_process_cpu_usage();
    
#elif defined(_WIN32)
    MEMORYSTATUSEX memory_status;
    memory_status.dwLength = sizeof(memory_status);
    if (GlobalMemoryStatusEx(&memory_status)) {
        usage.total_memory_mb = memory_status.ullTotalPhys / (1024 * 1024);
        usage.available_memory_mb = memory_status.ullAvailPhys / (1024 * 1024);
    }
    
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        usage.memory_usage_mb = static_cast<double>(pmc.WorkingSetSize) / (1024 * 1024);
    }
    
    usage.cpu_percent = get_process_cpu_usage();
#endif
    
    return usage;
}

uint64_t SystemInfo::get_process_memory_usage() {
#ifdef __linux__
    std::ifstream status_file("/proc/self/status");
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.find("VmRSS:") != std::string::npos) {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;
            std::string unit;
            iss >> key >> value >> unit;
            return value * 1024;
        }
    }
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#endif
    
    return 0;
}

double SystemInfo::get_process_cpu_usage() {
#ifdef __linux__
    std::ifstream stat_file("/proc/stat");
    std::string line;
    uint64_t total_time = 0;
    
    if (std::getline(stat_file, line)) {
        std::istringstream iss(line);
        std::string cpu;
        uint64_t user, nice, system, idle;
        iss >> cpu >> user >> nice >> system >> idle;
        total_time = user + nice + system + idle;
    }
    
    std::ifstream self_stat_file("/proc/self/stat");
    uint64_t process_time = 0;
    
    if (std::getline(self_stat_file, line)) {
        std::istringstream iss(line);
        std::string ignore;
        uint64_t utime, stime;
        for (int i = 0; i < 13; ++i) iss >> ignore;
        iss >> utime >> stime;
        process_time = utime + stime;
    }
    
    if (last_total_time_ > 0 && last_process_time_ > 0) {
        uint64_t total_diff = total_time - last_total_time_;
        uint64_t process_diff = process_time - last_process_time_;
        
        if (total_diff > 0) {
            double cpu_usage = 100.0 * process_diff / total_diff;
            last_total_time_ = total_time;
            last_process_time_ = process_time;
            return cpu_usage;
        }
    }
    
    last_total_time_ = total_time;
    last_process_time_ = process_time;
    
#elif defined(_WIN32)
    static ULARGE_INTEGER last_cpu_time, last_system_time;
    
    FILETIME now, creation_time, exit_time, kernel_time, user_time;
    GetSystemTimeAsFileTime(&now);
    
    if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER current_cpu_time, current_system_time;
        current_cpu_time.LowPart = user_time.dwLowDateTime + kernel_time.dwLowDateTime;
        current_cpu_time.HighPart = user_time.dwHighDateTime + kernel_time.dwHighDateTime;
        current_system_time.LowPart = now.dwLowDateTime;
        current_system_time.HighPart = now.dwHighDateTime;
        
        if (last_cpu_time.QuadPart > 0 && last_system_time.QuadPart > 0) {
            uint64_t cpu_diff = current_cpu_time.QuadPart - last_cpu_time.QuadPart;
            uint64_t system_diff = current_system_time.QuadPart - last_system_time.QuadPart;
            
            if (system_diff > 0) {
                double cpu_usage = 100.0 * cpu_diff / system_diff;
                last_cpu_time = current_cpu_time;
                last_system_time = current_system_time;
                return cpu_usage;
            }
        }
        
        last_cpu_time = current_cpu_time;
        last_system_time = current_system_time;
    }
#endif
    
    return 0.0;
}

std::string SystemInfo::get_hostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "unknown";
}

std::string SystemInfo::get_os_info() {
#ifdef __linux__
    std::ifstream os_release("/etc/os-release");
    std::string line;
    
    while (std::getline(os_release, line)) {
        if (line.find("PRETTY_NAME=") != std::string::npos) {
            return line.substr(13, line.length() - 14);
        }
    }
    return "Linux";
#elif defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Unknown";
#endif
}

} 