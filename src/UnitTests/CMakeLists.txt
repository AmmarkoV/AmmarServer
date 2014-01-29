project( UnitTests ) 
cmake_minimum_required( VERSION 2.8.7 )

set(CMAKE_CXX_FLAGS "-fPIC -march=native -mtune=native -O3 -fexpensive-optimizations -s") 
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(testHashMap testHashMap.c ) 
target_link_libraries(testHashMap pthread rt AmmarServer )
set_target_properties(testHashMap PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(testHashMap AmmarServer)   
