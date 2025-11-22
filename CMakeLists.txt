cmake_minimum_required(VERSION 3.25)

project(digital_clock_v3)

find_package(CURL REQUIRED)

add_executable(digital_clock_v3 main.cpp)

target_link_libraries(digital_clock_v3 PRIVATE CURL::libcurl)
