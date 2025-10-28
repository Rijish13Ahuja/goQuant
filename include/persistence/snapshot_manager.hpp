#ifndef SNAPSHOT_MANAGER_HPP
#define SNAPSHOT_MANAGER_HPP

#include "core/order_book.hpp"
#include <sqlite3.h>
#include <string>
#include <memory>
#include <vector>

namespace GoQuant {

struct OrderBookSnapshot {
    std::string symbol;
    uint64_t timestamp;
    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;
    
    OrderBookSnapshot(const std::string& sym, uint64_t ts) 
        : symbol(sym), timestamp(ts) {}
};

class SnapshotManager {
public:
    SnapshotManager(const std::string& db_path = "orderbook.db");
    ~SnapshotManager();
    
    bool initialize();
    bool save_snapshot(const OrderBookSnapshot& snapshot);
    bool save_snapshot(const std::string& symbol, 
                      const std::vector<std::pair<double, double>>& bids,
                      const std::vector<std::pair<double, double>>& asks);
    
    OrderBookSnapshot load_latest_snapshot(const std::string& symbol);
    std::vector<OrderBookSnapshot> load_snapshots(const std::string& symbol, 
                                                 uint64_t start_time, uint64_t end_time);
    
    void cleanup_old_snapshots(uint64_t retention_days = 7);

private:
    std::string db_path_;
    sqlite3* db_;
    
    bool create_tables();
    bool is_initialized() const { return db_ != nullptr; }
};

} 

#endif