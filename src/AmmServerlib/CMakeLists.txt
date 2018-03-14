project( AmmarServer ) 
cmake_minimum_required( VERSION 2.8.7 )
 
set(CMAKE_CXX_FLAGS "-s -Wl,-z,relro,-z,now -fstack-protector -O2 -fPIC ")
set(CMAKE_C_FLAGS "-s -Wl,-z,relro,-z,now -fstack-protector -O2 -fPIC ")

IF(DO_TEST_COVERAGE)
set(CMAKE_CXX_FLAGS " -fPIC -Wall -fprofile-arcs -ftest-coverage ")
set(CMAKE_C_FLAGS "-fPIC -Wall -fprofile-arcs -ftest-coverage")
ENDIF(DO_TEST_COVERAGE)
 
# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
 
set(AMMARSERVER_INGREDIENTS
                main.c 
                AmmServerlib.h
                server_configuration.c
                #AString string helper
                AString/AString.c
                AString/AString.h 
                #Caching mechanisms
                cache/file_caching.c
                cache/file_caching.h
                cache/dynamic_requests.c
                cache/dynamic_requests.h
                cache/client_list.c
                cache/client_list.h
                cache/session_list.c
                cache/session_list.h
                cache/file_compression.c
                cache/file_compression.h 
                #Scheduler Functionality 
                scheduler/scheduler.c  
                #Header analysis
                header_analysis/http_header_analysis.c
                header_analysis/http_header_analysis.h  
                header_analysis/post_header_analysis.c
                header_analysis/post_header_analysis.h
                header_analysis/generic_header_tools.c
                header_analysis/generic_header_tools.h
                header_analysis/post_data.c
                header_analysis/post_data.h
                header_analysis/get_data.c
                header_analysis/get_data.h
                header_analysis/cookie_data.c
                header_analysis/cookie_data.h
                #Network I/O
                network/networkAbstraction.c
                network/networkAbstraction.h
                network/openssl_server.c
                network/openssl_server.h
                network/file_server.c
                network/file_server.h
                network/sendHTTPHeader.c
                network/sendHTTPHeader.h
                network/recvHTTPHeader.c
                network/recvHTTPHeader.h
                #Threads 
                threads/threadInitHelper.h
                threads/freshThreads.c
                threads/freshThreads.h
                threads/prespawnedThreads.c
                threads/prespawnedThreads.h
                threads/clientServer.c
                threads/clientServer.h
                threads/threadedServer.c
                threads/threadedServer.h
                threads/libeventServer.c
                threads/libeventServer.h
                threads/serverAbstraction.c
                threads/serverAbstraction.h
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
                tools/geolocation.c
                tools/geolocation.h
                tools/time_provider.c
                tools/time_provider.h
                tools/http_tools.c
                tools/http_tools.h
                tools/serverMonitor.c
                tools/serverMonitor.h
                tools/directory_lists.c
                tools/directory_lists.h 
                #Security Functionality 
                security/banlist.c  
                #Templates that are used in clients..
                templates/editor.c 
                templates/editor.h 
                templates/login.c 
                templates/login.h 
                templates/errors.c 
                templates/errors.h 
                templates/icons.c 
                templates/icons.h 
              )
 
if (USE_LIBEVENT)
 set( LibEventParts  -levent )
 MESSAGE("LibEvent will be linked against AmmarServerLib")
ELSE (USE_LIBEVENT)
 MESSAGE("LibEvent will not be used against AmmarServerLib") 
ENDIF(USE_LIBEVENT)
 
if (USE_OPENSSL)
 set( OpenSSLParts  -lssl -lcrypto )
 MESSAGE("OpenSSL will be used against AmmarServerLib")
ELSE (USE_OPENSSL)
 MESSAGE("OpenSSL will not be used against AmmarServerLib") 
ENDIF(USE_OPENSSL)
  
    add_library(AmmarServerDynamic SHARED ${AMMARSERVER_INGREDIENTS} )
    target_link_libraries(AmmarServerDynamic pthread rt ${AmmarServer_LIBRARIES} ${OpenSSLParts} ${LibEventParts} Hashmap InputParser_C)

    add_library(AmmarServer STATIC ${AMMARSERVER_INGREDIENTS} )
    target_link_libraries(AmmarServer pthread rt ${AmmarServer_LIBRARIES} ${OpenSSLParts} ${LibEventParts} Hashmap InputParser_C)

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
 
