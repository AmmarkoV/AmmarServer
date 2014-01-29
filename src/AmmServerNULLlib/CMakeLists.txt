project( AmmarServerNull ) 
cmake_minimum_required( VERSION 2.8.7 )
set(CMAKE_CXX_FLAGS "-fPIC -march=native -mtune=native -O3 -fexpensive-optimizations -s")
# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
 
add_custom_command(
    TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ln -s ../AmmServerlib/AmmServerlib.h AmmServerlib.h 
)

#add_custom_command(OUTPUT AmmServerlib.h COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/custom.cmake)
add_custom_command(OUTPUT AmmServerlib.h COMMAND ln -s ../AmmServerlib/AmmServerlib.h AmmServerlib.h)

ADD_CUSTOM_TARGET(run echo "run a test" DEPENDS AmmarServer)

add_library(AmmarServerNullDynamic SHARED main.c AmmServerlib.h)
add_library(AmmarServerNull STATIC  main.c AmmServerlib.h) 
set_target_properties(AmmarServer PROPERTIES DEBUG_POSTFIX "D")
 
 
#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 
