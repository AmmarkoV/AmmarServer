project( ammarflix )
cmake_minimum_required( VERSION 3.10 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2 -D_FORTIFY_SOURCE=2")

IF(DO_TEST_COVERAGE)
set(CMAKE_C_FLAGS "-fPIE -Wall -fprofile-arcs -ftest-coverage")
ENDIF(DO_TEST_COVERAGE)

add_executable(ammarflix
 main.c
 library.c
 library.h
 media.c
 media.h
 pages.c
 pages.h
 )
target_link_libraries(ammarflix pthread rt AmmarServerDynamic UserAccounts )
add_dependencies(ammarflix AmmarServer UserAccounts)

set_target_properties(ammarflix PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                     )
