project( AmmarServerNull ) 
cmake_minimum_required( VERSION 2.8.7 )
set(CMAKE_CXX_FLAGS "-fPIC -march=native -mtune=native -O3 -fexpensive-optimizations -s")
# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   

#Left here as a reminder
#add_custom_command(OUTPUT AmmServerlib.h COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/custom.cmake)

add_custom_command(OUTPUT AmmServerlib.h COMMAND ln -fs  ${CMAKE_SOURCE_DIR}/src/AmmServerlib/AmmServerlib.h ${CMAKE_SOURCE_DIR}/src/AmmServerNULLlib/AmmServerlib.h ) 
 

add_library(AmmarServerNullDynamic SHARED main.c AmmServerlib.h)
add_library(AmmarServerNull STATIC  main.c AmmServerlib.h) 
set_target_properties(AmmarServer PROPERTIES DEBUG_POSTFIX "D")
 
 
#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 
