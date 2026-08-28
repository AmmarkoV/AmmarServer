###########################################################
# AmmarServer , the cmake files ;P
#
#
# ---------------------------------------------------------------------------------------------------------- #
#
project( supermarket )
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2 -D_FORTIFY_SOURCE=2")

IF(DO_TEST_COVERAGE)
set(CMAKE_C_FLAGS "-fPIE -Wall -fprofile-arcs -ftest-coverage")
ENDIF(DO_TEST_COVERAGE)

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})

add_executable(supermarket
 main.c
 state.c
 state.h
 json.c
 json.h
 photo.c
 photo.h
 page.c
 page.h
 )
target_link_libraries(supermarket pthread rt m AmmarServerDynamic BasicImaging )
set_target_properties(supermarket PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(supermarket AmmarServer BasicImaging)


    set_target_properties(supermarket PROPERTIES
                          DEBUG_POSTFIX ""
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
