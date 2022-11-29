cmake_minimum_required(VERSION 3.18)
project(http-server-unity C)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR ../../src)

include(${SRC_DIR}/utils.cmake)

add_subdirectory_lazy(${SRC_DIR}/audit malachi-iot::http-audit)
add_subdirectory_lazy(${SRC_DIR}/http malachi-iot::http-server)
add_subdirectory_lazy(${SRC_DIR}/fileserver malachi-iot::http-fileserver)
add_subdirectory_lazy(${SRC_DIR}/tokenizer malachi-iot::tokenizer)
add_subdirectory_lazy(${EXT_DIR}/Unity unity)

# DEBT: This doesn't seem like quite the right way to activate this feature, but it works
target_compile_definitions(unity PRIVATE UNITY_INCLUDE_PRINT_FORMATTED)

# DEBT: Yes, we were naughty and mixed convention using both _ and - for separators.  That needs attention

add_executable(${PROJECT_NAME}
        array_test.c
        audit_tests.c
        domain-socket.c
        experimental_test.c
        http_fileserver_tests.c
        http_header_test.c
        http_pipeline_tests.c
        http_request_test.c
        http_response_tests.c
        main.c
        queue-tests.c
        tokenizer_tests.c
        thread-tests.c
        )

add_executable(malachi-iot::http-server-unity ALIAS ${PROJECT_NAME})

# TODO: Get things working even in this environment - right now unused variables
# and gnu extensions make it mad
#target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)


target_link_libraries(${PROJECT_NAME} unity)
target_link_libraries(${PROJECT_NAME} malachi-iot::http-audit malachi-iot::c_containers malachi-iot::tokenizer)
target_link_libraries(${PROJECT_NAME} malachi-iot::http-server malachi-iot::http-fileserver)
