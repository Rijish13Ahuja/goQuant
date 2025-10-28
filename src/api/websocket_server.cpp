#include "api/websocket_server.hpp"
#include "api/json_serializer.hpp"
#include "utils/uuid_generator.hpp"
#include <iostream>
#include <sstream>
namespace GoQuant
{
    WebSocketServer::WebSocketServer(MatchingEngine &engine, int port)
        : engine_(engine), port_(port)
    {

        app_ = std::make_unique<uWS::App>();

        app_->ws<PerSocketData>("/*", {.idleTimeout = 120,
                                       .maxBackpressure = 1024 * 1024,
                                       .open = [this](auto *ws)
                                       { std::cout << "Client connected" << std::endl; },
                                       .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode)
                                       {
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
                } else {
                    ErrorResponse error{"invalid_message", "Unknown message type"};
                    send_message(ws, JsonSerializer::serialize_error_response(error));
                }
            } catch (const std::exception& e) {
                ErrorResponse error{"parse_error", e.what()};
                send_message(ws, JsonSerializer::serialize_error_response(error));
            } },
                                       .close = [this](auto *ws, int code, std::string_view message)
                                       {
            std::cout << "Client disconnected" << std::endl;
            unsubscribe_all(ws); }});

        app_->get("/health", [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req)
                  {
        res->writeStatus("200 OK");
        res->writeHeader("Content-Type", "application/json");
        res->end(R"({"status": "healthy"})"); });
    }
    WebSocketServer::~WebSocketServer()
    {
        stop();
    }
    void WebSocketServer::start()
    {
        if (running_)
            return;

        running_ = true;
        server_thread_ = std::thread(&WebSocketServer::run_server, this);

        std::cout << "WebSocket server starting on port " << port_ << std::endl;
    }
    void WebSocketServer::stop()
    {
        if (!running_)
            return;

        running_ = false;
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }
    void WebSocketServer::run_server()
    {
        app_->listen(port_, [this](auto *listen_socket)
                     {
        if (listen_socket) {
            std::cout << "WebSocket server listening on port " << port_ << std::endl;
        } else {
            std::cerr << "Failed to start WebSocket server on port " << port_ << std::endl;
        } });

        app_->run();
    }
    void WebSocketServer::handle_order_request(uWS::WebSocket<false, true> *ws, const std::string &message)
    {
        try
        {
            OrderRequest request = JsonSerializer::parse_order_request(message);

            if (request.symbol.empty() || request.order_type.empty() || request.side.empty() || request.quantity <= 0)
            {
                ErrorResponse error{"invalid_request", "Missing or invalid required fields"};
                send_message(ws, JsonSerializer::serialize_error_response(error));
                return;
            }

            OrderType order_type;
            if (request.order_type == "market")
                order_type = OrderType::MARKET;
            else if (request.order_type == "limit")
                order_type = OrderType::LIMIT;
            else if (request.order_type == "ioc")
                order_type = OrderType::IOC;
            else if (request.order_type == "fok")
                order_type = OrderType::FOK;
            else
            {
                ErrorResponse error{"invalid_order_type", "Unknown order type: " + request.order_type};
                send_message(ws, JsonSerializer::serialize_error_response(error));
                return;
            }

            OrderSide side;
            if (request.side == "buy")
                side = OrderSide::BUY;
            else if (request.side == "sell")
                side = OrderSide::SELL;
            else
            {
                ErrorResponse error{"invalid_side", "Unknown side: " + request.side};
                send_message(ws, JsonSerializer::serialize_error_response(error));
                return;
            }

            std::string order_id = request.order_id.empty() ? UUIDGenerator::generate() : request.order_id;

            uint64_t timestamp = JsonSerializer::get_current_timestamp();
            Order order(order_id, request.symbol, order_type, side, request.quantity, request.price, timestamp);

            bool success = engine_.submit_order(order);

            if (success)
            {
                OrderResponse response(order_id, "accepted", "Order accepted", order.filled_quantity, 0.0);
                send_message(ws, JsonSerializer::serialize_order_response(response));
            }
            else
            {
                OrderResponse response(order_id, "rejected", "Order rejected", 0.0, 0.0);
                send_message(ws, JsonSerializer::serialize_order_response(response));
            }
        }
        catch (const std::exception &e)
        {
            ErrorResponse error{"order_error", e.what()};
            send_message(ws, JsonSerializer::serialize_error_response(error));
        }
    }
    void WebSocketServer::handle_cancel_request(uWS::WebSocket<false, true> *ws, const std::string &message)
    {
        try
        {
            CancelRequest request = JsonSerializer::parse_cancel_request(message);

            if (request.symbol.empty() || request.order_id.empty())
            {
                ErrorResponse error{"invalid_request", "Missing symbol or order_id"};
                send_message(ws, JsonSerializer::serialize_error_response(error));
                return;
            }

            bool success = engine_.cancel_order(request.symbol, request.order_id);

            if (success)
            {
                OrderResponse response(request.order_id, "cancelled", "Order cancelled");
                send_message(ws, JsonSerializer::serialize_order_response(response));
            }
            else
            {
                ErrorResponse error{"cancel_failed", "Order not found or already filled"};
                send_message(ws, JsonSerializer::serialize_error_response(error));
            }
        }
        catch (const std::exception &e)
        {
            ErrorResponse error{"cancel_error", e.what()};
            send_message(ws, JsonSerializer::serialize_error_response(error));
        }
    }
    void WebSocketServer::handle_market_data_request(uWS::WebSocket<false, true> *ws, const std::string &message)
    {
        try
        {
            MarketDataRequest request = JsonSerializer::parse_market_data_request(message);

            if (request.symbol.empty() || request.type.empty())
            {
                ErrorResponse error{"invalid_request", "Missing symbol or subscription type"};
                send_message(ws, JsonSerializer::serialize_error_response(error));
                return;
            }

            if (request.type == "bbo")
            {
                subscribe_bbo(ws, request.symbol);
            }
            else if (request.type == "depth")
            {
                subscribe_depth(ws, request.symbol);
            }
            else if (request.type == "trades")
            {
                subscribe_trades(ws, request.symbol);
            }
            else
            {
                ErrorResponse error{"invalid_subscription", "Unknown subscription type: " + request.type};
                send_message(ws, JsonSerializer::serialize_error_response(error));
                return;
            }

            auto book = engine_.get_order_book(request.symbol);
            if (book)
            {
                if (request.type == "bbo")
                {
                    auto bbo_msg = JsonSerializer::serialize_bbo_update(
                        request.symbol, book->get_best_bid(), book->get_best_ask(),
                        JsonSerializer::get_current_timestamp());
                    send_message(ws, bbo_msg);
                }
                else if (request.type == "depth")
                {
                    auto bids = book->get_bid_levels(10);
                    auto asks = book->get_ask_levels(10);
                    auto depth_msg = JsonSerializer::serialize_order_book_update(
                        request.symbol, bids, asks, JsonSerializer::get_current_timestamp());
                    send_message(ws, depth_msg);
                }
            }
        }
        catch (const std::exception &e)
        {
            ErrorResponse error{"subscription_error", e.what()};
            send_message(ws, JsonSerializer::serialize_error_response(error));
        }
    }
    void WebSocketServer::send_message(uWS::WebSocket<false, true> *ws, const std::string &message)
    {
        ws->send(message, uWS::OpCode::TEXT);
    }
    void WebSocketServer::broadcast_market_data(const std::string &symbol, const std::string &message)
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);

        auto bbo_it = bbo_subscribers_.find(symbol);
        if (bbo_it != bbo_subscribers_.end())
        {
            for (auto *ws : bbo_it->second)
            {
                send_message(ws, message);
            }
        }

        auto depth_it = depth_subscribers_.find(symbol);
        if (depth_it != depth_subscribers_.end())
        {
            for (auto *ws : depth_it->second)
            {
                send_message(ws, message);
            }
        }
    }
    void WebSocketServer::broadcast_trade(const Trade &trade)
    {
        std::string trade_msg = JsonSerializer::serialize_trade(trade);

        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        auto it = trade_subscribers_.find(trade.symbol);
        if (it != trade_subscribers_.end())
        {
            for (auto *ws : it->second)
            {
                send_message(ws, trade_msg);
            }
        }
    }
    void WebSocketServer::subscribe_bbo(uWS::WebSocket<false, true> *ws, const std::string &symbol)
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        bbo_subscribers_[symbol].insert(ws);
    }
    void WebSocketServer::subscribe_depth(uWS::WebSocket<false, true> *ws, const std::string &symbol)
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        depth_subscribers_[symbol].insert(ws);
    }
    void WebSocketServer::subscribe_trades(uWS::WebSocket<false, true> *ws, const std::string &symbol)
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        trade_subscribers_[symbol].insert(ws);
    }
    void WebSocketServer::unsubscribe_all(uWS::WebSocket<false, true> *ws)
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);

        for (auto &[symbol, subscribers] : bbo_subscribers_)
        {
            subscribers.erase(ws);
        }

        for (auto &[symbol, subscribers] : depth_subscribers_)
        {
            subscribers.erase(ws);
        }

        for (auto &[symbol, subscribers] : trade_subscribers_)
        {
            subscribers.erase(ws);
        }
    }
}