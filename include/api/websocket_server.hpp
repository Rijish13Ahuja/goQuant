#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include "core/matching_engine.hpp"
#include "api/message_types.hpp"
#include <uwebsockets/App.h>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <functional>

namespace GoQuant
{

    class WebSocketServer
    {
    public:
        WebSocketServer(MatchingEngine &engine, int port = 9001);
        ~WebSocketServer();

        void start();
        void stop();
        void broadcast_market_data(const std::string &symbol, const std::string &message);
        void broadcast_trade(const Trade &trade);

    private:
        MatchingEngine &engine_;
        int port_;
        std::atomic<bool> running_{false};
        std::thread server_thread_;

        std::unique_ptr<uWS::App> app_;

        std::unordered_map<std::string, std::unordered_set<uWS::WebSocket<false, true> *>> bbo_subscribers_;
        std::unordered_map<std::string, std::unordered_set<uWS::WebSocket<false, true> *>> depth_subscribers_;
        std::unordered_map<std::string, std::unordered_set<uWS::WebSocket<false, true> *>> trade_subscribers_;
        std::mutex subscribers_mutex_;

        void run_server();
        void handle_order_request(uWS::WebSocket<false, true> *ws, const std::string &message);
        void handle_cancel_request(uWS::WebSocket<false, true> *ws, const std::string &message);
        void handle_market_data_request(uWS::WebSocket<false, true> *ws, const std::string &message);

        void send_message(uWS::WebSocket<false, true> *ws, const std::string &message);

        void subscribe_bbo(uWS::WebSocket<false, true> *ws, const std::string &symbol);
        void subscribe_depth(uWS::WebSocket<false, true> *ws, const std::string &symbol);
        void subscribe_trades(uWS::WebSocket<false, true> *ws, const std::string &symbol);
        void unsubscribe_all(uWS::WebSocket<false, true> *ws);
    };

}

#endif