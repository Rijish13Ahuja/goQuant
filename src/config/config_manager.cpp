#include "config/config_manager.hpp"
#include <fstream>
#include <iostream>

namespace GoQuant {

ConfigManager& ConfigManager::get_instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load_config(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    try {
        if (!std::filesystem::exists(config_path)) {
            std::cout << "Config file not found, using defaults: " << config_path << std::endl;
            init_default_config();
            return save_config(config_path);
        }
        
        std::ifstream file(config_path);
        nlohmann::json config_json;
        file >> config_json;
        
        engine_config_ = EngineConfig::from_json(config_json["engine"]);
        
        symbol_configs_.clear();
        for (const auto& symbol_json : config_json["symbols"]) {
            SymbolConfig symbol_config = SymbolConfig::from_json(symbol_json);
            symbol_configs_[symbol_config.symbol] = symbol_config;
        }
        
        std::cout << "Configuration loaded from: " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        init_default_config();
        return false;
    }
}

bool ConfigManager::save_config(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    try {
        std::filesystem::create_directories(std::filesystem::path(config_path).parent_path());
        
        nlohmann::json config_json;
        config_json["engine"] = engine_config_.to_json();
        
        config_json["symbols"] = nlohmann::json::array();
        for (const auto& [symbol, config] : symbol_configs_) {
            config_json["symbols"].push_back(config.to_json());
        }
        
        std::ofstream file(config_path);
        file << config_json.dump(4);
        
        std::cout << "Configuration saved to: " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save config: " << e.what() << std::endl;
        return false;
    }
}

EngineConfig ConfigManager::get_engine_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return engine_config_;
}

SymbolConfig ConfigManager::get_symbol_config(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    auto it = symbol_configs_.find(symbol);
    if (it != symbol_configs_.end()) {
        return it->second;
    }
    return SymbolConfig(symbol);
}

std::vector<SymbolConfig> ConfigManager::get_all_symbol_configs() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    std::vector<SymbolConfig> configs;
    for (const auto& [symbol, config] : symbol_configs_) {
        configs.push_back(config);
    }
    return configs;
}

void ConfigManager::set_engine_config(const EngineConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    engine_config_ = config;
}

void ConfigManager::set_symbol_config(const SymbolConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    symbol_configs_[config.symbol] = config;
}

void ConfigManager::validate_order(const std::string& symbol, double price, double quantity) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = symbol_configs_.find(symbol);
    if (it == symbol_configs_.end()) {
        throw std::runtime_error("Unknown symbol: " + symbol);
    }
    
    const auto& config = it->second;
    
    if (price < config.min_price || price > config.max_price) {
        throw std::runtime_error("Price out of range for symbol: " + symbol);
    }
    
    if (quantity < config.min_quantity || quantity > config.max_quantity) {
        throw std::runtime_error("Quantity out of range for symbol: " + symbol);
    }
    
    if (std::fmod(price, config.price_tick) != 0.0) {
        throw std::runtime_error("Price must be multiple of tick size: " + std::to_string(config.price_tick));
    }
    
    if (std::fmod(quantity, config.quantity_step) != 0.0) {
        throw std::runtime_error("Quantity must be multiple of step size: " + std::to_string(config.quantity_step));
    }
}

bool ConfigManager::is_valid_symbol(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return symbol_configs_.find(symbol) != symbol_configs_.end();
}

void ConfigManager::init_default_config() {
    engine_config_ = EngineConfig();
    init_default_symbols();
}

void ConfigManager::init_default_symbols() {
    symbol_configs_ = {
        {"BTC-USDT", SymbolConfig("BTC-USDT", 0.01, 1000000.0, 0.0001, 1000.0, 0.01, 0.0001)},
        {"ETH-USDT", SymbolConfig("ETH-USDT", 0.01, 100000.0, 0.001, 10000.0, 0.01, 0.001)},
        {"ADA-USDT", SymbolConfig("ADA-USDT", 0.001, 100.0, 1.0, 1000000.0, 0.001, 1.0)},
        {"DOT-USDT", SymbolConfig("DOT-USDT", 0.01, 1000.0, 0.1, 100000.0, 0.01, 0.1)},
        {"LINK-USDT", SymbolConfig("LINK-USDT", 0.01, 1000.0, 0.1, 100000.0, 0.01, 0.1)}
    };
}

nlohmann::json EngineConfig::to_json() const {
    nlohmann::json j;
    j["websocket_port"] = websocket_port;
    j["max_connections"] = max_connections;
    j["order_book_depth"] = order_book_depth;
    j["maker_fee"] = maker_fee;
    j["taker_fee"] = taker_fee;
    j["enable_persistence"] = enable_persistence;
    j["persistence_path"] = persistence_path;
    j["snapshot_interval_seconds"] = snapshot_interval_seconds;
    j["max_order_size"] = max_order_size;
    j["price_tick_size"] = price_tick_size;
    j["quantity_step"] = quantity_step;
    j["enable_advanced_orders"] = enable_advanced_orders;
    j["performance_stats_interval"] = performance_stats_interval;
    return j;
}

EngineConfig EngineConfig::from_json(const nlohmann::json& j) {
    EngineConfig config;
    config.websocket_port = j.value("websocket_port", 9001);
    config.max_connections = j.value("max_connections", 10000);
    config.order_book_depth = j.value("order_book_depth", 10);
    config.maker_fee = j.value("maker_fee", 0.001);
    config.taker_fee = j.value("taker_fee", 0.002);
    config.enable_persistence = j.value("enable_persistence", true);
    config.persistence_path = j.value("persistence_path", "data/");
    config.snapshot_interval_seconds = j.value("snapshot_interval_seconds", 60);
    config.max_order_size = j.value("max_order_size", 1000.0);
    config.price_tick_size = j.value("price_tick_size", 0.01);
    config.quantity_step = j.value("quantity_step", 0.001);
    config.enable_advanced_orders = j.value("enable_advanced_orders", true);
    config.performance_stats_interval = j.value("performance_stats_interval", 5);
    return config;
}

nlohmann::json SymbolConfig::to_json() const {
    nlohmann::json j;
    j["symbol"] = symbol;
    j["min_price"] = min_price;
    j["max_price"] = max_price;
    j["min_quantity"] = min_quantity;
    j["max_quantity"] = max_quantity;
    j["price_tick"] = price_tick;
    j["quantity_step"] = quantity_step;
    return j;
}

SymbolConfig SymbolConfig::from_json(const nlohmann::json& j) {
    return SymbolConfig(
        j.value("symbol", ""),
        j.value("min_price", 0.0),
        j.value("max_price", 1000000.0),
        j.value("min_quantity", 0.001),
        j.value("max_quantity", 10000.0),
        j.value("price_tick", 0.01),
        j.value("quantity_step", 0.001)
    );
}

} 