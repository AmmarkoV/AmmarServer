###########################################################
# AmmarServer , the cmake files ;P
#
# BasicImaging : thumbnailing / safe JPEG-PNG-PZP encode-decode for every Service to share.
# JPEG, PNG and PZP support are each individually optional : if their backing library ( libjpeg ,
# libpng , or BOTH libzstd+liblz4 for PZP ) isn't found on this machine, that codec is compiled OUT
# ( not stubbed with dummy symbols ) , a warning is printed at configure time , and
# BasicImaging_HasJPEG()/HasPNG()/HasPZP() plus the Save calls' return values are how callers find out
# at runtime - they never crash either way.
# ---------------------------------------------------------------------------------------------------------- #
#
project( BasicImaging )
cmake_minimum_required( VERSION 2.8.7 )

# Extend cmake's module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules ${CMAKE_MODULE_PATH})

set(CMAKE_C_FLAGS "-fPIC -O2")

find_package(JPEG)
find_package(PNG)

#zstd and lz4 have no official upstream CMake Find module ( unlike JPEG/PNG ) , so detect them the
#classic way : look for the header and the library separately.
find_path(ZSTD_INCLUDE_DIR zstd.h)
find_library(ZSTD_LIBRARY NAMES zstd)
find_path(LZ4_INCLUDE_DIR lz4.h)
find_library(LZ4_LIBRARY NAMES lz4)

set(BASICIMAGING_SOURCES
    basicImaging.c
    basicImaging.h
    resize.c
    codecs/codecs.c
    codecs/codecs.h
    codecs/image.h
    codecs/ppmInput.c
    codecs/ppmInput.h
   )

set(BASICIMAGING_DEFINITIONS "")
set(BASICIMAGING_LIBRARIES "")
set(BASICIMAGING_INCLUDES "")

if (JPEG_FOUND)
  message(STATUS "BasicImaging: libjpeg found (${JPEG_LIBRARIES}) , JPEG support ENABLED")
  list(APPEND BASICIMAGING_SOURCES codecs/jpgInput.c codecs/jpgInput.h)
  list(APPEND BASICIMAGING_DEFINITIONS USE_JPG_FILES=1)
  list(APPEND BASICIMAGING_LIBRARIES ${JPEG_LIBRARIES})
  list(APPEND BASICIMAGING_INCLUDES ${JPEG_INCLUDE_DIR})
else()
  message(WARNING "BasicImaging: libjpeg/jpeglib.h NOT found -- JPEG support DISABLED , install libjpeg-dev (or equivalent) and re-run cmake to enable it. BasicImaging_SaveJPEG()/BasicImaging_HasJPEG() will report this at runtime instead of crashing.")
  list(APPEND BASICIMAGING_DEFINITIONS USE_JPG_FILES=0)
endif()

if (PNG_FOUND)
  message(STATUS "BasicImaging: libpng found (${PNG_LIBRARIES}) , PNG support ENABLED")
  list(APPEND BASICIMAGING_SOURCES codecs/pngInput.c codecs/pngInput.h)
  list(APPEND BASICIMAGING_DEFINITIONS USE_PNG_FILES=1)
  list(APPEND BASICIMAGING_LIBRARIES ${PNG_LIBRARIES})
  list(APPEND BASICIMAGING_INCLUDES ${PNG_INCLUDE_DIRS})
else()
  message(WARNING "BasicImaging: libpng/png.h NOT found -- PNG support DISABLED , install libpng-dev (or equivalent) and re-run cmake to enable it. BasicImaging_SavePNG()/BasicImaging_HasPNG() will report this at runtime instead of crashing.")
  list(APPEND BASICIMAGING_DEFINITIONS USE_PNG_FILES=0)
endif()

#PZP needs BOTH libzstd and liblz4 ; either one missing disables it entirely, same as JPEG/PNG above.
if (ZSTD_INCLUDE_DIR AND ZSTD_LIBRARY AND LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
  message(STATUS "BasicImaging: libzstd (${ZSTD_LIBRARY}) and liblz4 (${LZ4_LIBRARY}) found , PZP support ENABLED")
  list(APPEND BASICIMAGING_SOURCES codecs/pzp.h codecs/pzpInput.c codecs/pzpInput.h)
  list(APPEND BASICIMAGING_DEFINITIONS USE_PZP_FILES=1)
  list(APPEND BASICIMAGING_LIBRARIES ${ZSTD_LIBRARY} ${LZ4_LIBRARY})
  list(APPEND BASICIMAGING_INCLUDES ${ZSTD_INCLUDE_DIR} ${LZ4_INCLUDE_DIR})
else()
  message(WARNING "BasicImaging: libzstd/zstd.h and/or liblz4/lz4.h NOT found -- PZP support DISABLED , install libzstd-dev and liblz4-dev and re-run cmake to enable it. BasicImaging_SavePZP()/BasicImaging_HasPZP() will report this at runtime instead of crashing.")
  list(APPEND BASICIMAGING_DEFINITIONS USE_PZP_FILES=0)
endif()

include_directories(${BASICIMAGING_INCLUDES})

add_library(BasicImagingDynamic SHARED ${BASICIMAGING_SOURCES})
add_library(BasicImaging STATIC ${BASICIMAGING_SOURCES})

target_compile_definitions(BasicImagingDynamic PUBLIC ${BASICIMAGING_DEFINITIONS})
target_compile_definitions(BasicImaging        PUBLIC ${BASICIMAGING_DEFINITIONS})

target_link_libraries(BasicImagingDynamic ${BASICIMAGING_LIBRARIES} m)
target_link_libraries(BasicImaging        ${BASICIMAGING_LIBRARIES} m)

     set_target_properties(BasicImagingDynamic PROPERTIES
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )
     set_target_properties(BasicImaging PROPERTIES
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

add_executable(testbasicimaging testBasicImaging.c)
target_link_libraries(testbasicimaging pthread rt m BasicImaging)
set_target_properties(testbasicimaging PROPERTIES DEBUG_POSTFIX "D")

    set_target_properties(testbasicimaging PROPERTIES
                          DEBUG_POSTFIX "D"
                          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                         )

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
