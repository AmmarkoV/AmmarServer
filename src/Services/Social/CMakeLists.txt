project( social ) 
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2")

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(social main.c login.c chat.c home.c) 
target_link_libraries(social pthread rt AmmarServer UserAccounts )
set_target_properties(social PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(social AmmarServer)  
       
    set_target_properties(social PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

