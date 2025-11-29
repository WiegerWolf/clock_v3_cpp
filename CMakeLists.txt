cmake_minimum_required(VERSION 3.25)

project(digital_clock_v3)

find_package(cpr REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

add_executable(digital_clock_v3 main.cpp)

target_link_libraries(digital_clock_v3
    PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
        SDL3::SDL3
)
