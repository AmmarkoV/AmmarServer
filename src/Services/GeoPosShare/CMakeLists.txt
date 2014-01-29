project( geoposshare ) 
cmake_minimum_required( VERSION 2.8.7 )

set(CMAKE_CXX_FLAGS "-fPIC -march=native -mtune=native -O3 -fexpensive-optimizations -s")
#set(CMAKE_CXX_COMPILER "g++-4.6")

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(geoposshare main.c ) 
target_link_libraries(geoposshare pthread rt AmmarServer )
set_target_properties(geoposshare PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(geoposshare AmmarServer)  
       


#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

