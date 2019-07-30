#examples
cmake_minimum_required (VERSION 2.6)
project (libkindrv_exampels)

# LIBKINDRV entries should only be shown in advanced view
mark_as_advanced(LIBKINDRV_LIB LIBKINDRV_INCLUDE_DIR)

# find installed kindrv library and include dir
set (LIBKINDRV_FOUND FALSE CACHE STRING "FALSE" FORCE)

include (FindPkgConfig OPTIONAL)
if (PKG_CONFIG_FOUND)
  # find libkindrv with pkg-config (recommended)
  pkg_check_modules (LIBKINDRV REQUIRED libkindrv)

else (PKG_CONFIG_FOUND)
  message (WARNING "FindPkgConfig not found, cannot use pkg-config to detect"
                   " libkindrv. Using custom cmake code to find libkinrv"
                   " and adding boost dependency (not recommended)")

  # find libkindrv
  # no cmake module available yet, therefore these lines instead of
  #  simply using "find_package(libkindrv REQUIRED)"
  find_library (LIBKINDRV_LIBRARIES kindrv HINTS ENV LD_LIBRARY_PATH)
  find_path (LIBKINDRV_INCLUDE_DIRS kindrv.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
    HINTS
      ENV CPATH
    PATH_SUFFIXES
      libkindrv
  )

  if (LIBKINDRV_LIBRARIES AND LIBKINDRV_INCLUDE_DIRS)
    set (LIBKINDRV_FOUND TRUE CACHE STRING "TRUE" FORCE)
  endif (LIBKINDRV_LIBRARIES AND LIBKINDRV_INCLUDE_DIRS)

  # find boost-system
  find_package (Boost REQUIRED COMPONENTS system)
  set (LIBKINDRV_LIBRARIES ${LIBKINDRV_LIBRARIES} ${Boost_LIBRARIES})
  set (LIBKINDRV_INCLUDE_DIRS ${LIBKINDRV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
endif (PKG_CONFIG_FOUND)

if (LIBKINDRV_FOUND)
  include_directories (${LIBKINDRV_INCLUDE_DIRS})
  if (LIBKINDRV_LIBRARY_DIRS)
    link_directories (${LIBKINDRV_LIBRARY_DIRS})
  endif (LIBKINDRV_LIBRARY_DIRS)
else (LIBKINDRV_FOUND)
  message(FATAL_ERROR "Could not find libkindrv!")
endif (LIBKINDRV_FOUND)

add_subdirectory (AmmarServer/)


add_executable (libkindrv_ex_simple ex_simple.cpp)
add_executable (libkindrv_ex_ctrl ex_ctrl.cpp)
add_executable (libkindrv_ex_joystick ex_joystick.cpp)
add_executable (libkindrv_ex_dualarm ex_dualarm.cpp)
add_executable (libkindrv_ammarserver ammarserver.cpp)
 
target_link_libraries (libkindrv_ex_simple ${LIBKINDRV_LIBRARIES})
target_link_libraries (libkindrv_ex_ctrl ${LIBKINDRV_LIBRARIES})
target_link_libraries (libkindrv_ex_joystick ${LIBKINDRV_LIBRARIES})
target_link_libraries (libkindrv_ex_dualarm ${LIBKINDRV_LIBRARIES}  )
target_link_libraries (libkindrv_ammarserver ${LIBKINDRV_LIBRARIES} pthread rt AmmarServer)
