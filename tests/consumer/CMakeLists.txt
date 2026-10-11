# Minimal out-of-tree consumer: proves the installed package works via find_package().
cmake_minimum_required(VERSION 3.14)
project(InsightDockConsumer LANGUAGES CXX)

# Intentionally request an older standard: the imported target must raise this to C++17.
set(CMAKE_CXX_STANDARD 11)
find_package(InsightDock REQUIRED)

add_executable(consumer main.cc)
target_link_libraries(consumer PRIVATE InsightDock::Charts)
