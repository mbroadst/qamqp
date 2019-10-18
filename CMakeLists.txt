cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

project(qamqp LANGUAGES CXX VERSION 0.5.0)

get_filename_component(QAMQP_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
include(${QAMQP_PATH}/cmake/CMakeSettings.cmake)

add_subdirectory(src)
add_subdirectory(tests/auto)
