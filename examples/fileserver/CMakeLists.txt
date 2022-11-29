cmake_minimum_required(VERSION 3.18)
project(fileserver C)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR ../../src)

include(${SRC_DIR}/utils.cmake)

add_subdirectory_lazy(${SRC_DIR}/audit malachi-iot::http-audit)
add_subdirectory_lazy(${SRC_DIR}/fileserver malachi-iot::http-fileserver)

add_executable(${PROJECT_NAME} http.c main.c)

target_link_libraries(${PROJECT_NAME} malachi-iot::http-audit malachi-iot::c_containers malachi-iot::http-fileserver)
