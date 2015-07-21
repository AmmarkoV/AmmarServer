###########################################################
# AmmarServer , the cmake files ;P
#
#
# ---------------------------------------------------------------------------------------------------------- #
#
project( habchan ) 
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2")


# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(habchan 
 ../../AmmServerlib/hashmap/hashmap.c 
 ../../AmmServerlib/hashmap/hashmap.h 
 ../../AmmServerlib/InputParser/InputParser_C.c  
 ../../AmmServerlib/InputParser/InputParser_C.h 
 main.c 
 thread.c 
 thread.h 
 board.c 
 board.h 
 state.c 
 state.h 
 postReceiver.c
 postReceiver.h
 ) 
target_link_libraries(habchan pthread rt AmmarServer )
set_target_properties(habchan PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(habchan AmmarServer)  
       

    set_target_properties(habchan PROPERTIES 
                          DEBUG_POSTFIX ""
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
