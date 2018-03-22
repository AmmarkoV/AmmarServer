project( AmmMessagesTestServer ) 
cmake_minimum_required( VERSION 2.8.7 )

set(CMAKE_CXX_FLAGS "-fPIC -march=native -mtune=native -O3 -fexpensive-optimizations -s") 
# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(AmmMessagesTestServer testServer.c mmapBridge.c mmapBridge.h)  
target_link_libraries(AmmMessagesTestServer pthread rt AmmarServer)

add_executable(AmmMessagesTestClient testClient.c mmapBridge.c mmapBridge.h)  
target_link_libraries(AmmMessagesTestClient pthread rt AmmClient)

add_executable(AmmMessagesTestMMAPClientAtServer testClientAtSameMachineAsServer.c mmapBridge.c mmapBridge.h)   
        

set_target_properties(AmmMessagesTestServer PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
set_target_properties(AmmMessagesTestClient PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
set_target_properties(AmmMessagesTestMMAPClientAtServer PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )



#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

