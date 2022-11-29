function(add_subdirectory_lazy FOLDER TARGET)
    #set(BINARY_DIR ${ARGV2})
    #message(DEBUG "BINARY_DIR=${BINARY_DIR}")
    set(BINARY_DIR ${TARGET})

    # Makefiles can't handle : in binary dir names
    # Ninja seems to be able to
    if(${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
        string(REPLACE ":" "_" BINARY_DIR ${BINARY_DIR})
    endif()
    if(NOT TARGET ${TARGET})
        add_subdirectory(${FOLDER} ${BINARY_DIR})
    endif()
endfunction()

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
set(EXT_DIR ${ROOT_DIR}/ext)

add_subdirectory_lazy(${EXT_DIR} pglx::ext)

add_compile_options(-Werror=return-type)
# This helps assert() not generate warnings
add_compile_options(-Wno-unused-value)
