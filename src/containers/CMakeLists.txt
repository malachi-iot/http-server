cmake_minimum_required(VERSION 3.18)
project(c_containers C)

set(CMAKE_C_STANDARD 11)

include(../utils.cmake)

# See pglx-8 for ESP_PLATFORM info
if(NOT ESP_PLATFORM)
    # Gets 3rd party logger
    # DEBT: slightly concerned that they install with the super generic name 'logger'
    add_subdirectory_lazy(${EXT_DIR}/c-logger logger_static)

    # DEBT: Want to do non-static logger, and I think to do so we have to find_package
    #find_package(logger REQUIRED)
    # pthreads needs this [6] - also according to [6] clang can do this with a -pthreads compiler option
    find_package(Threads REQUIRED)
endif()


set(SOURCE
        array.h
        bind.h
        queue.h
        threadpool.h
        array.c
        bind.c
        queue.c
        string.c
        threadpool.c
        )

add_library(${PROJECT_NAME}_shared SHARED ${SOURCE})
add_library(${PROJECT_NAME} STATIC ${SOURCE})

add_library(malachi-iot::c_containers ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC .)
target_include_directories(${PROJECT_NAME}_shared PUBLIC .)

if(NOT ESP_PLATFORM)
    message(DEBUG "c_containers for Desktop")
    # Almost works, just doesn't like include_next
    #target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)

    target_link_libraries(${PROJECT_NAME}_shared Threads::Threads logger)
    target_link_libraries(${PROJECT_NAME} Threads::Threads logger_static)
else()
    message(DEBUG "c_containers for esp-idf")
    target_include_directories(${PROJECT_NAME} PUBLIC esp-idf)
    target_include_directories(${PROJECT_NAME}_shared PUBLIC esp-idf)
endif()

#target_link_libraries(${PROJECT_NAME} malachi-iot::queue)
#target_link_libraries(${PROJECT_NAME}_shared malachi-iot::queue)


