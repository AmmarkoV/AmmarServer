project( asvio )
cmake_minimum_required( VERSION 3.10 )

#Server applications need all the protection we can get..!
set(CMAKE_C_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2 -D_FORTIFY_SOURCE=2 -Wall")

#libasvio.so links the static AmmarServer so the python/asvio package is self-contained
add_library(asvio SHARED bridge/asvio.c )
target_link_libraries(asvio AmmarServer pthread rt )
add_dependencies(asvio AmmarServer)

set_target_properties(asvio PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python/asvio"
                     )
