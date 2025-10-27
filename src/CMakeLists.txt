add_executable(matching_engine
    main.cpp
    core/order_book.cpp
    core/matching_engine.cpp
)
target_link_libraries(matching_engine 
    PRIVATE 
)

target_include_directories(matching_engine PRIVATE ${CMAKE_SOURCE_DIR}/include)