cmake_minimum_required(VERSION 3.18)
project(pglx-10-tokenizer C)

set(CMAKE_C_STANDARD 11)

include(../utils.cmake)

add_subdirectory_lazy(../containers malachi-iot::c_containers)

add_library(${PROJECT_NAME}_shared SHARED tokenizer.c)
add_library(${PROJECT_NAME} STATIC tokenizer.c)

add_library(malachi-iot::tokenizer ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME}_shared malachi-iot::queue malachi-iot::c_containers)
target_link_libraries(${PROJECT_NAME} malachi-iot::queue malachi-iot::c_containers)