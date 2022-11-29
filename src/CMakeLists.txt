cmake_minimum_required(VERSION 3.18)
project(pglx-10 C)

set(CMAKE_C_STANDARD 11)

include(utils.cmake)

add_subdirectory_lazy(audit pglx-10::audit)
add_subdirectory_lazy(fileserver malachi-iot::http-fileserver)

add_executable(${PROJECT_NAME} http.c main.c README.md)

target_link_libraries(${PROJECT_NAME} pglx-10::audit malachi-iot::c_containers malachi-iot::http-fileserver)
