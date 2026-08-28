###########################################################
# AmmarServer , the cmake files ;P
#
#
# ---------------------------------------------------------------------------------------------------------- #
#
project( sharetex )
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2 -D_FORTIFY_SOURCE=2")

IF(DO_TEST_COVERAGE)
set(CMAKE_C_FLAGS "-fPIE -Wall -fprofile-arcs -ftest-coverage")
ENDIF(DO_TEST_COVERAGE)

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})

add_executable(sharetex
 main.c
 state.c
 state.h
 auth.c
 auth.h
 project.c
 project.h
 editor.c
 editor.h
 cursor.c
 cursor.h
 compile.c
 compile.h
 )
target_link_libraries(sharetex pthread rt m AmmarServerDynamic UserAccounts )
set_target_properties(sharetex PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(sharetex AmmarServer UserAccounts)


    set_target_properties(sharetex PROPERTIES
                          DEBUG_POSTFIX ""
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
