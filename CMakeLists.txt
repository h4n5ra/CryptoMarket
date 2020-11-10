cmake_minimum_required(VERSION 3.0.0)
project(CryptoMarket VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include(CTest)
enable_testing()

add_executable(CryptoMarket src/main.cpp src/Market.cpp src/Market.h src/jsoncpp.cpp)
