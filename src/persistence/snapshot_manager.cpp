#include "persistence/snapshot_manager.hpp"
#include <iostream>

namespace GoQuant {

SnapshotManager::SnapshotManager(const std::string& db_path) 
    : db_path_(db_path), db_(nullptr) {}

SnapshotManager::~SnapshotManager() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool SnapshotManager::initialize() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    return create_tables();
}

bool SnapshotManager::create_tables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS orderbook_snapshots (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            bids TEXT NOT NULL,
            asks TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE INDEX IF NOT EXISTS idx_symbol_timestamp ON orderbook_snapshots(symbol, timestamp);
    )";
    
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool SnapshotManager::save_snapshot(const OrderBookSnapshot& snapshot) {
    return save_snapshot(snapshot.symbol, snapshot.bids, snapshot.asks);
}

bool SnapshotManager::save_snapshot(const std::string& symbol, 
                                   const std::vector<std::pair<double, double>>& bids,
                                   const std::vector<std::pair<double, double>>& asks) {
    if (!is_initialized()) return false;
    
    nlohmann::json bids_json = nlohmann::json::array();
    for (const auto& [price, qty] : bids) {
        nlohmann::json level = nlohmann::json::array();
        level.push_back(price);
        level.push_back(qty);
        bids_json.push_back(level);
    }
    
    nlohmann::json asks_json = nlohmann::json::array();
    for (const auto& [price, qty] : asks) {
        nlohmann::json level = nlohmann::json::array();
        level.push_back(price);
        level.push_back(qty);
        asks_json.push_back(level);
    }
    
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    const char* sql = "INSERT INTO orderbook_snapshots (symbol, timestamp, bids, asks) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, timestamp);
    sqlite3_bind_text(stmt, 3, bids_json.dump().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, asks_json.dump().c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

OrderBookSnapshot SnapshotManager::load_latest_snapshot(const std::string& symbol) {
    OrderBookSnapshot snapshot(symbol, 0);
    
    if (!is_initialized()) return snapshot;
    
    const char* sql = "SELECT timestamp, bids, asks FROM orderbook_snapshots WHERE symbol = ? ORDER BY timestamp DESC LIMIT 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return snapshot;
    
    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snapshot.timestamp = sqlite3_column_int64(stmt, 0);
        
        const char* bids_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* asks_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        auto bids_data = nlohmann::json::parse(bids_json);
        auto asks_data = nlohmann::json::parse(asks_json);
        
        for (const auto& level : bids_data) {
            snapshot.bids.emplace_back(level[0].get<double>(), level[1].get<double>());
        }
        
        for (const auto& level : asks_data) {
            snapshot.asks.emplace_back(level[0].get<double>(), level[1].get<double>());
        }
    }
    
    sqlite3_finalize(stmt);
    return snapshot;
}

std::vector<OrderBookSnapshot> SnapshotManager::load_snapshots(const std::string& symbol, 
                                                              uint64_t start_time, 
                                                              uint64_t end_time) {
    std::vector<OrderBookSnapshot> snapshots;
    
    if (!is_initialized()) return snapshots;
    
    const char* sql = "SELECT timestamp, bids, asks FROM orderbook_snapshots WHERE symbol = ? AND timestamp BETWEEN ? AND ? ORDER BY timestamp";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return snapshots;
    
    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, start_time);
    sqlite3_bind_int64(stmt, 3, end_time);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OrderBookSnapshot snapshot(symbol, sqlite3_column_int64(stmt, 0));
        
        const char* bids_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* asks_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        auto bids_data = nlohmann::json::parse(bids_json);
        auto asks_data = nlohmann::json::parse(asks_json);
        
        for (const auto& level : bids_data) {
            snapshot.bids.emplace_back(level[0].get<double>(), level[1].get<double>());
        }
        
        for (const auto& level : asks_data) {
            snapshot.asks.emplace_back(level[0].get<double>(), level[1].get<double>());
        }
        
        snapshots.push_back(snapshot);
    }
    
    sqlite3_finalize(stmt);
    return snapshots;
}

void SnapshotManager::cleanup_old_snapshots(uint64_t retention_days) {
    if (!is_initialized()) return;
    
    uint64_t cutoff_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - (retention_days * 24 * 60 * 60 * 1000);
    
    const char* sql = "DELETE FROM orderbook_snapshots WHERE timestamp < ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return;
    
    sqlite3_bind_int64(stmt, 1, cutoff_time);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} 