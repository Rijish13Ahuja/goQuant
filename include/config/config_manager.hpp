#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <mutex>
#include <filesystem>

namespace GoQuant {

struct EngineConfig {
    int websocket_port = 9001;
    int max_connections = 10000;
    int order_book_depth = 10;
    double maker_fee = 0.001;
    double taker_fee = 0.002;
    bool enable_persistence = true;
    std::string persistence_path = "data/";
    int snapshot_interval_seconds = 60;
    int max_order_size = 1000.0;
    double price_tick_size = 0.01;
    double quantity_step = 0.001;
    bool enable_advanced_orders = true;
    int performance_stats_interval = 5;
    
    nlohmann::json to_json() const;
    static EngineConfig from_json(const nlohmann::json& j);
};

struct SymbolConfig {
    std::string symbol;
    double min_price;
    double max_price;
    double min_quantity;
    double max_quantity;
    double price_tick;
    double quantity_step;
    
    SymbolConfig(const std::string& sym = "", double min_p = 0.0, double max_p = 1000000.0,
                 double min_q = 0.001, double max_q = 10000.0, double tick = 0.01, double step = 0.001)
        : symbol(sym), min_price(min_p), max_price(max_p), min_quantity(min_q),
          max_quantity(max_q), price_tick(tick), quantity_step(step) {}
    
    nlohmann::json to_json() const;
    static SymbolConfig from_json(const nlohmann::json& j);
};

class ConfigManager {
public:
    static ConfigManager& get_instance();
    
    bool load_config(const std::string& config_path = "config/default.json");
    bool save_config(const std::string& config_path = "config/default.json");
    
    EngineConfig get_engine_config() const;
    SymbolConfig get_symbol_config(const std::string& symbol) const;
    std::vector<SymbolConfig> get_all_symbol_configs() const;
    
    void set_engine_config(const EngineConfig& config);
    void set_symbol_config(const SymbolConfig& config);
    
    void validate_order(const std::string& symbol, double price, double quantity) const;
    bool is_valid_symbol(const std::string& symbol) const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    
    EngineConfig engine_config_;
    std::unordered_map<std::string, SymbolConfig> symbol_configs_;
    mutable std::mutex config_mutex_;
    
    void init_default_config();
    void init_default_symbols();
};

} 

#endif