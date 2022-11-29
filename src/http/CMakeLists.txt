cmake_minimum_required(VERSION 3.18)
project(http-server C)

set(CMAKE_C_STANDARD 11)

include(../utils.cmake)

add_subdirectory_lazy(../containers malachi-iot::c_containers)
add_subdirectory_lazy(../tokenizer malachi-iot::tokenizer)

set(INCLUDE_FILES
        http/pipeline.h
        http/request.h
        http/response.h
        )

add_library(${PROJECT_NAME}
        debug.c
        http_context.c
        http_header.c
        http_pipeline.c
        http_request.c
        http_request_decoder.c
        http_request_posix.c
        http_request_statemachine.c
        http_response.c
        http_threadpool.c
        ${INCLUDE_FILES}
        )


add_library(malachi-iot::http-server ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} malachi-iot::c_containers malachi-iot::tokenizer)