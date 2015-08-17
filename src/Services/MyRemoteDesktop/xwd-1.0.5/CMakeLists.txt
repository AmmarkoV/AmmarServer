project( xwd ) 
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2")

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   

set(XWD_INGREDIENTS clientwin.c clientwin.h dsimple.c dsimple.h list.c list.h multiVis.c multiVis.h config.h wsutils.h )
   
add_library(XwdLib STATIC ${XWD_INGREDIENTS} main.c )
add_executable(xwd ${XWD_INGREDIENTS} xwd.c ) 
target_link_libraries(xwd X11 )
set_target_properties(xwd PROPERTIES DEBUG_POSTFIX "D")


       
    set_target_properties(xwd PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

