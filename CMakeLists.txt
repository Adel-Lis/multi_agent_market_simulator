cmake_minimum_required(VERSION 3.20)
project(cda_sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- the engine: compiled once, shared by every program -------------------
add_library(cda_core STATIC
        src/Price.cpp
        src/OrderBook.cpp
        src/Agent.cpp
        include/Order.hpp
        include/OrderBook.hpp
        include/Price.hpp
        include/Random.hpp
        include/RingBuffer.hpp
        include/Config.hpp
        include/MarketState.hpp
        include/Agent.hpp
        include/Simulation.hpp
        src/Simulation.cpp
        src/Config.cpp
)
target_include_directories(cda_core PUBLIC include)
target_compile_options(cda_core PRIVATE -Wall -Wextra)

# --- the simulator --------------------------------------------------------
add_executable(sim src/main.cpp)
target_link_libraries(sim PRIVATE cda_core)
target_compile_options(sim PRIVATE -Wall -Wextra)

# --- tests: one executable per file with a main ---------------------------
add_executable(test_orderbook tests/test_orderbook.cpp tests/TestFramework.hpp)
target_link_libraries(test_orderbook PRIVATE cda_core)
target_include_directories(test_orderbook PRIVATE tests)
target_compile_options(test_orderbook PRIVATE -Wall -Wextra)

add_executable(test_agent tests/test_agent.cpp tests/TestFramework.hpp)
target_link_libraries(test_agent PRIVATE cda_core)
target_include_directories(test_agent PRIVATE tests)
target_compile_options(test_agent PRIVATE -Wall -Wextra)

enable_testing()
add_test(NAME orderbook COMMAND test_orderbook)
add_test(NAME agent COMMAND test_agent)