cmake_minimum_required(VERSION 3.18)
project(http-fileserver C)

set(CMAKE_C_STANDARD 11)

include(../utils.cmake)

add_subdirectory_lazy(../audit pglx-10::audit)
add_subdirectory_lazy(../http malachi-iot::http-server)

add_library(${PROJECT_NAME}
        audit.c
        fileserver.c
        fileserver-socket.c
        fileserver-stream.c
        )

add_library(malachi-iot::http-fileserver ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC .)
target_link_libraries(${PROJECT_NAME} pglx-10::audit malachi-iot::c_containers malachi-iot::http-server)
