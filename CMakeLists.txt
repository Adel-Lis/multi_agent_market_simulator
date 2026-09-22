cmake_minimum_required(VERSION 4.0)
project(cda_sim)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(cda_sim
        include/Order.hpp
        include/OrderBook.hpp
        include/Price.hpp
        include/Random.hpp
        include/RingBuffer.hpp
        src/main.cpp
        src/OrderBook.cpp
        src/Price.cpp)

add_executable(test_orderbook
        tests/test_orderbook.cpp
        src/OrderBook.cpp
        src/Price.cpp
)
