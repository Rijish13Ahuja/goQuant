include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9fdab5992fd8b7ee1ae2d96.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(test_order_book 
    test_order_book.cpp 
    ../src/core/order_book.cpp
    ../src/core/trade.cpp
)

add_executable(test_matching_engine 
    test_matching_engine.cpp 
    ../src/core/matching_engine.cpp
    ../src/core/order_book.cpp
    ../src/core/trade.cpp
)

target_link_libraries(test_order_book gtest_main)
target_link_libraries(test_matching_engine gtest_main)

add_test(NAME order_book_test COMMAND test_order_book)
add_test(NAME matching_engine_test COMMAND test_matching_engine)