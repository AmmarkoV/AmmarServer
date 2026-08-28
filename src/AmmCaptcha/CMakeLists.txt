project( AmmCaptcha ) 
cmake_minimum_required( VERSION 2.8.7 )

#this is a "heavy task" it needs optimizations 
set(CMAKE_C_FLAGS "-fPIC -O3 -fexpensive-optimizations -s") 

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
       )

    add_library(AmmCaptchaDynamic SHARED ${AMMCAPTCHA_INGREDIENTS} )
    add_library(AmmCaptcha STATIC ${AMMCAPTCHA_INGREDIENTS} )
    #JPEG support now comes from BasicImaging ( jpeg arrives transitively through it ) ; AmmCaptcha is
    #added before BasicImaging in the top-level CMakeLists so the add_dependencies below matter.
    target_link_libraries(AmmCaptcha BasicImaging m Hashmap)
    target_link_libraries(AmmCaptchaDynamic BasicImagingDynamic m)
    add_dependencies(AmmCaptcha BasicImaging)
    add_dependencies(AmmCaptchaDynamic BasicImaging)
       
 
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
 

