project( AmmClient ) 
cmake_minimum_required( VERSION 2.8.7 )
 
set(CMAKE_CXX_FLAGS "-s -Wl,-z,relro,-z,now -fstack-protector -O2 -fPIC ")
set(CMAKE_C_FLAGS "-s -Wl,-z,relro,-z,now -fstack-protector -O2 -fPIC ")
# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
 
   
    set(AmmClient_INGREDIENTS 
                tools.c
                network.c
                protocol.c
                AmmClient.c
              )


if (USE_OPENSSL)
 set( OpenSSLParts  -lssl -lcrypto )
 MESSAGE("OpenSSL will be used")
ELSE (USE_OPENSSL)
 MESSAGE("OpenSSL will not be used") 
ENDIF(USE_OPENSSL)
  
    add_library(AmmClientDynamic SHARED ${AmmClient_INGREDIENTS} )
    target_link_libraries(AmmClientDynamic pthread rt ${AmmClient_LIBRARIES} ${OpenSSLParts} )

    add_library(AmmClient STATIC ${AmmClient_INGREDIENTS} )
    target_link_libraries(AmmClient pthread rt ${AmmClient_LIBRARIES} ${OpenSSLParts})



    set_target_properties(AmmClient PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
  
    set_target_properties(AmmClientDynamic PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          OUTPUT_NAME AmmClient 
                        )


 
#    install(TARGETS AmmClient
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 
