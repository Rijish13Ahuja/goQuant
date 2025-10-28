add_executable(matching_engine
    main.cpp
    core/order_book.cpp
    core/matching_engine.cpp
    core/trade.cpp
    core/advanced_orders.cpp
    api/websocket_server.cpp
    api/json_serializer.cpp
    market_data/market_data_feed.cpp
    persistence/snapshot_manager.cpp
    persistence/event_logger.cpp
    fees/fee_calculator.cpp
    utils/logger.cpp
    utils/uuid_generator.cpp
    utils/benchmark.cpp
    utils/performance_counter.cpp
)

target_link_libraries(matching_engine 
    PRIVATE 
    Threads::Threads
    uWebSockets
    nlohmann_json::nlohmann_json
    sqlite3
)

target_compile_definitions(matching_engine PRIVATE NDEBUG)