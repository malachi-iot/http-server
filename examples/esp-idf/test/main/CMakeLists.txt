idf_component_register(SRCS "test.c" "wifi.c"
                    INCLUDE_DIRS ".")

# Guidance from
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#using-esp-idf-in-custom-cmake-projects

SET(HTTP_SRC_FOLDER ../../../src)
add_subdirectory(${HTTP_SRC_FOLDER}/http http)
add_subdirectory_lazy(${HTTP_SRC_FOLDER}/pipeline malachi-iot::http-fileserver)

# Nearly there, but has issues with shared vs static linking
target_link_libraries(${COMPONENT_LIB} malachi-iot::c_containers malachi-iot::http-server malachi-iot::http-fileserver)
