project( AmmarServer ) 
cmake_minimum_required( VERSION 2.8.7 )
 
set(CMAKE_CXX_FLAGS "-s -Wl,-z,relro,-z,now -fstack-protector -O2 -fPIC ")
set(CMAKE_C_FLAGS "-s -Wl,-z,relro,-z,now -fstack-protector -O2 -fPIC ")
# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
 
   
    set(AMMARSERVER_INGREDIENTS
                main.c 
                AmmServerlib.h
                server_configuration.c
                #AString string helper
                AString/AString.c
                AString/AString.h
                #InputParser
                InputParser/InputParser_C.c 
                InputParser/InputParser_C.h 
                #Caching mechanisms
                cache/file_caching.c
                cache/file_caching.h
                cache/dynamic_requests.c
                cache/dynamic_requests.h
                cache/client_list.c
                cache/client_list.h
                cache/file_compression.c
                cache/file_compression.h
                #Hashing mechanisms
                hashmap/hashmap.c
                hashmap/hashmap.h
                #Header analysis
                header_analysis/http_header_analysis.c
                header_analysis/http_header_analysis.h  
                header_analysis/post_header_analysis.c
                header_analysis/post_header_analysis.h
                #Network I/O
                network/file_server.c
                network/file_server.h
                network/sendHTTPHeader.c
                network/sendHTTPHeader.h
                network/recvHTTPHeader.c
                network/recvHTTPHeader.h
                #Threads
                threads/threadInitHelper.c
                threads/threadInitHelper.h
                threads/freshThreads.c
                threads/freshThreads.h
                threads/prespawnedThreads.c
                threads/prespawnedThreads.h
                threads/clientServer.c
                threads/clientServer.h
                threads/threadedServer.c
                threads/threadedServer.h
                #String Scanners , these get  automatically generated
                stringscanners/applicationFiles.c
                stringscanners/applicationFiles.h
                stringscanners/archiveFiles.c
                stringscanners/archiveFiles.h
                stringscanners/audioFiles.c
                stringscanners/audioFiles.h
                stringscanners/firstLines.c
                stringscanners/firstLines.h
                stringscanners/httpHeader.c
                stringscanners/httpHeader.h
                stringscanners/imageFiles.c
                stringscanners/imageFiles.h
                stringscanners/postHeader.c
                stringscanners/postHeader.h
                stringscanners/textFiles.c
                stringscanners/textFiles.h
                stringscanners/videoFiles.c
                stringscanners/videoFiles.h
                #Various Tools 
                tools/logs.c
                tools/logs.h
                tools/time_provider.c
                tools/time_provider.h
                tools/http_tools.c
                tools/http_tools.h
                tools/serverMonitor.c
                tools/serverMonitor.h
                tools/directory_lists.c
                tools/directory_lists.h 
              )
  
    add_library(AmmarServerDynamic SHARED ${AMMARSERVER_INGREDIENTS} )
    target_link_libraries(AmmarServerDynamic pthread rt ${AmmarServer_LIBRARIES})

    add_library(AmmarServer STATIC ${AMMARSERVER_INGREDIENTS} )
    target_link_libraries(AmmarServer pthread rt ${AmmarServer_LIBRARIES})



    set_target_properties(AmmarServer PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
  
    set_target_properties(AmmarServerDynamic PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          OUTPUT_NAME AmmarServer 
                        )


 
#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 
