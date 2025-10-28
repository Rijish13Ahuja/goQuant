add_executable(matching_engine
    main.cpp
    core/order_book.cpp
    core/matching_engine.cpp
    core/trade.cpp
    api/websocket_server.cpp
    api/json_serializer.cpp
    market_data/market_data_feed.cpp
    utils/logger.cpp
    utils/uuid_generator.cpp
)

target_link_libraries(matching_engine 
    PRIVATE 
    Threads::Threads
    uWebSockets
    nlohmann_json::nlohmann_json
)