cmake_minimum_required(VERSION 3.18)
project(pglx-8 C)

set(CMAKE_C_STANDARD 11)

# pthreads needs this [6] - also according to [6] clang can do this with a -pthreads compiler option
find_package(Threads REQUIRED)

add_subdirectory(.. lib)
add_subdirectory(../../../ext/Unity Unity)

add_executable(${PROJECT_NAME}
        main.c
        thread-tests.c
        thread-tests.h
        )

# DEBT: This doesn't seem like quite the right way to activate this feature, but it works
target_compile_definitions(unity PRIVATE UNITY_INCLUDE_PRINT_FORMATTED)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)

target_link_libraries(${PROJECT_NAME} pglx-8::lib unity Threads::Threads)