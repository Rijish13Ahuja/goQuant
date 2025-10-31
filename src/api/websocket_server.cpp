#include "api/websocket_server.hpp"
#include "api/json_serializer.hpp"
#include "utils/uuid_generator.hpp"
#include "config/config_manager.hpp"
#include <iostream>
#include <sstream>

namespace GoQuant {

WebSocketServer::WebSocketServer(MatchingEngine& engine, int port) 
    : engine_(engine), port_(port) {
    
    auto config = ConfigManager::get_instance().get_engine_config();
    if (port == 9001) {
        port_ = config.websocket_port;
    }
    
    app_ = std::make_unique<uWS::App>();
    
    app_->ws<PerSocketData>("/*", {
        .idleTimeout = 120,
        .maxBackpressure = 1024 * 1024,
        .maxPayloadLength = 16 * 1024 * 1024,
        .open = [this](auto* ws) {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            active_connections_++;
            std::cout << "Client connected. Total connections: " << active_connections_ << std::endl;
        },
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) {
            try {
                std::string msg_str(message);
                auto j = nlohmann::json::parse(msg_str);
                std::string message_type = j.value("type", "");
                
                if (message_type == "order") {
                    handle_order_request(ws, msg_str);
                } else if (message_type == "cancel") {
                    handle_cancel_request(ws, msg_str);
                } else if (message_type == "subscribe") {
                    handle_market_data_request(ws, msg_str);
                } else if (message_type == "unsubscribe") {
                    handle_unsubscribe_request(ws, msg_str);
                } else {
                    ErrorResponse error{"invalid_message", "Unknown message type"};
                    send_message(ws, JsonSerializer::serialize_error_response(error));
                }
            } catch (const std::exception& e) {
                ErrorResponse error{"parse_error", e.what()};
                send_message(ws, JsonSerializer::serialize_error_response(error));
            }
        },
        .close = [this](auto* ws, int code, std::string_view message) {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            active_connections_ = std::max(0, active_connections_ - 1);
            std::cout << "Client disconnected. Total connections: " << active_connections_ << std::endl;
            unsubscribe_all(ws);
        }
    });
    
    app_->get("/health", [this](uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
        nlohmann::json health_status;
        health_status["status"] = "healthy";
        health_status["timestamp"] = JsonSerializer::get_current_timestamp();
        health_status["connections"] = active_connections_;
        health_status["version"] = "1.0.0";
        
        res->writeStatus("200 OK");
        res->writeHeader("Content-Type", "application/json");
        res->end(health_status.dump());
    });
    
    app_->get("/metrics", [this](uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
        nlohmann::json metrics;
        metrics["throughput_ops"] = engine_.get_throughput_ops();
        metrics["total_orders"] = engine_.get_orders_processed();
        metrics["active_connections"] = active_connections_;
        metrics["timestamp"] = JsonSerializer::get_current_timestamp();
        
        res->writeStatus("200 OK");
        res->writeHeader("Content-Type", "application/json");
        res->end(metrics.dump());
    });
}

void WebSocketServer::handle_unsubscribe_request(uWS::WebSocket<false, true>* ws, const std::string& message) {
    try {
        MarketDataRequest request = JsonSerializer::parse_market_data_request(message);
        
        if (request.symbol.empty() || request.type.empty()) {
            ErrorResponse error{"invalid_request", "Missing symbol or subscription type"};
            send_message(ws, JsonSerializer::serialize_error_response(error));
            return;
        }
        
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        
        if (request.type == "bbo") {
            auto it = bbo_subscribers_.find(request.symbol);
            if (it != bbo_subscribers_.end()) {
                it->second.erase(ws);
            }
        } else if (request.type == "depth") {
            auto it = depth_subscribers_.find(request.symbol);
            if (it != depth_subscribers_.end()) {
                it->second.erase(ws);
            }
        } else if (request.type == "trades") {
            auto it = trade_subscribers_.find(request.symbol);
            if (it != trade_subscribers_.end()) {
                it->second.erase(ws);
            }
        }
        
        nlohmann::json response;
        response["type"] = "unsubscribe_ack";
        response["symbol"] = request.symbol;
        response["subscription_type"] = request.type;
        response["timestamp"] = JsonSerializer::get_current_timestamp();
        
        send_message(ws, response.dump());
        
    } catch (const std::exception& e) {
        ErrorResponse error{"unsubscribe_error", e.what()};
        send_message(ws, JsonSerializer::serialize_error_response(error));
    }
}

} 