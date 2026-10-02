# Minimal out-of-tree consumer: proves the installed package works via find_package().
cmake_minimum_required(VERSION 3.14)
project(InsightDockConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(InsightDock REQUIRED)

add_executable(consumer main.cc)
target_link_libraries(consumer PRIVATE InsightDock::Charts)
