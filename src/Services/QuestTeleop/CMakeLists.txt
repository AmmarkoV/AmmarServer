project( questteleop ) 
cmake_minimum_required( VERSION 2.8.7 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIE -pie -s -Wl,-z,relro,-z,now -fstack-protector -O2 -D_FORTIFY_SOURCE=2")

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
   
add_executable(questteleop main.c ) 
target_link_libraries(questteleop pthread rt AmmarServerDynamic )
set_target_properties(questteleop PROPERTIES DEBUG_POSTFIX "D")
add_dependencies(questteleop AmmarServer)  

# Optional same-host link to the robot side through SharedMemoryVideoBuffers ( a checkout next to AmmarServer ,
# as in HumanoidController/dependencies/ ) ; raw frames from it are JPEG-encoded with BasicImaging
set(SHMVB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../SharedMemoryVideoBuffers" CACHE PATH "SharedMemoryVideoBuffers checkout")
if (EXISTS "${SHMVB_DIR}/src/c/sharedMemoryVideoBuffers.h")
  message(STATUS "questteleop: shared memory link with ${SHMVB_DIR}")
  target_sources(questteleop PRIVATE shm_link.c "${SHMVB_DIR}/src/c/sharedMemoryVideoBuffers.c")
  target_include_directories(questteleop PRIVATE "${SHMVB_DIR}/src/c")
  target_compile_definitions(questteleop PRIVATE USE_SHMVB=1)
  target_link_libraries(questteleop BasicImaging m)
  add_dependencies(questteleop BasicImaging)
else()
  message(STATUS "questteleop: no SharedMemoryVideoBuffers at ${SHMVB_DIR} , HTTP only")
endif()
       
    set_target_properties(questteleop PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

