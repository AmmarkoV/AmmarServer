project( UserAccounts ) 
cmake_minimum_required( VERSION 3.10 )

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
set(CMAKE_C_FLAGS "-fPIC ")

add_library(UserAccountsDynamic SHARED userAccounts.c userAccounts.h userAccountsWeb.c userAccountsWeb.h sha256.c sha256.h)
add_library(UserAccounts STATIC  userAccounts.c userAccounts.h userAccountsWeb.c userAccountsWeb.h sha256.c sha256.h)

#Command line tool to reset the password of an existing user
add_executable(ammarserver_passwd ammarserver_passwd.c)
target_link_libraries(ammarserver_passwd UserAccounts)
set_target_properties(ammarserver_passwd PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

#Command line tool to add a new user
add_executable(ammarserver_adduser ammarserver_adduser.c)
target_link_libraries(ammarserver_adduser UserAccounts)
set_target_properties(ammarserver_adduser PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

#Regression tests , work in a temporary directory only
add_executable(testuseraccounts testUserAccounts.c userAccounts.c sha256.c)
set_target_properties(testuseraccounts PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
 
     set_target_properties(UserAccountsDynamic PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
     set_target_properties(UserAccounts PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 
