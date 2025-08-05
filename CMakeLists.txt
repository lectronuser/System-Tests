cmake_minimum_required(VERSION 3.8)
project(system_checker)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};$ENV{LECTRON_DRIVERS};$ENV{LECTRON_LIBS};$ENV{MAVLINK_PATH}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(MAVLink REQUIRED)
find_package(gpio REQUIRED)
find_package(logger REQUIRED)
find_package(serial REQUIRED)

execute_process(COMMAND cat /proc/cpuinfo 
    COMMAND grep "Raspberry Pi"
    OUTPUT_VARIABLE RPI_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

if(RPI_FOUND)
    message(STATUS "Running on a Raspberry Pi")
    add_compile_definitions(USING_RASPBERRY_PI)
else()
    message(STATUS "Not a Raspberry Pi device")
endif()

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} 
    gpio
    logger
    serial
    MAVLink::mavlink
)