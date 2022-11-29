cmake_minimum_required(VERSION 3.18)
project(http-audit C)

set(CMAKE_C_STANDARD 11)

add_subdirectory_lazy(../http malachi-iot::http-server)

add_library(${PROJECT_NAME} audit.c pipeline.c)

add_library(malachi-iot::http-audit ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} malachi-iot::http-server)