project( AmmCaptcha ) 
cmake_minimum_required( VERSION 2.8.7 )

#this is a "heavy task" it needs optimizations 
set(CMAKE_C_FLAGS "-fPIC -march=native -mtune=native -O3 -fexpensive-optimizations -s") 

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})
 
 
# -lrt
#-pthreads
 
   
    set(AMMCAPTCHA_INGREDIENTS
        main.c          
        AmmCaptcha.h       
        imaging.c
        imaging.h
        img_warp.c
        img_warp.h
        jpgInput.c
        jpgInput.h
        ../AmmServerlib/hashmap/hashmap.c
        ../AmmServerlib/hashmap/hashmap.h
       )
  
    add_library(AmmCaptchaDynamic SHARED ${AMMCAPTCHA_INGREDIENTS} )
    add_library(AmmCaptcha STATIC ${AMMCAPTCHA_INGREDIENTS} )
    target_link_libraries(AmmCaptcha jpeg m)
    target_link_libraries(AmmCaptchaDynamic jpeg m) 
       
 
    set_target_properties(AmmCaptcha PROPERTIES 
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 

