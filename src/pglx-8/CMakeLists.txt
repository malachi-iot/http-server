cmake_minimum_required(VERSION 3.18)
project(pglx-8-lib C)

set(CMAKE_C_STANDARD 11)

if(NOT ESP_PLATFORM)
    # pthreads needs this [6] - also according to [6] clang can do this with a -pthreads compiler option
    find_package(Threads REQUIRED)
else()
    # esp-idf find_package doesn't work as expected, but pthreads are already available
    message(DEBUG "Compiling for esp-idf")
endif()

add_library(${PROJECT_NAME} STATIC
        queue.c
        queue.h
        )

add_library(pglx-8::lib ALIAS ${PROJECT_NAME})
add_library(malachi-iot::queue ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC .)

if(NOT ESP_PLATFORM)
    # This works well on desktops but embedded systems bend some rules
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()