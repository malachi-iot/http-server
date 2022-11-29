# Stub project for ext libraries which don't have CMakeLists
# (none so far)

project(pglx-ext)

add_library(${PROJECT_NAME} INTERFACE)

add_library(pglx::ext ALIAS ${PROJECT_NAME})