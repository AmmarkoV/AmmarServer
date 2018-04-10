###########################################################
# ammbus , the cmake files ;P
#
#
# ---------------------------------------------------------------------------------------------------------- #
#
project( ammbus ) 
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2")

IF(DO_TEST_COVERAGE)
set(CMAKE_C_FLAGS "-fPIE -Wall -fprofile-arcs -ftest-coverage")
ENDIF(DO_TEST_COVERAGE)

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(ammbus main.c serialIO.c state.c) 
target_link_libraries(ammbus pthread rt AmmarServerDynamic )
#set_target_properties(ammbus PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(ammbus ammbus)  
       

    set_target_properties(ammbus PROPERTIES 
                          #DEBUG_POSTFIX ""
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
#    install(TARGETS ammbus
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

 #   install(DIRECTORY ${PROJECT_SOURCE_DIR} DESTINATION include
 #       FILES_MATCHING PATTERN "*.h"
 #                      PATTERN "*.hpp"
 #                      PATTERN "*.inl"
 #                      PATTERN "build" EXCLUDE
 #                      PATTERN ".svn"  EXCLUDE) 
