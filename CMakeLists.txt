project( AmmarServerProject ) 
cmake_minimum_required( VERSION 2.8.7 )

#We are a "serious" server application and need all the stack protection we can get..! 
set(CMAKE_CXX_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2")
set(CMAKE_C_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2")

    #This is the core library 
    add_subdirectory (src/AmmServerlib)
 
    #After we are done with the basics make all our secondary libraries
    add_subdirectory (src/AmmCaptcha) 
    add_subdirectory (src/StringRecognizer)


    #Then make all of the executables that use the various libraries  
    add_subdirectory (src/Services/AmmarServer) 
    add_subdirectory (src/Services/ScriptRunner) 
    add_subdirectory (src/Services/MyURL) 
    add_subdirectory (src/Services/MyLoader) 
    add_subdirectory (src/Services/MyBlog) 
    add_subdirectory (src/Services/MyTube)
    add_subdirectory (src/Services/MySearch)
    #XWD is listed here maybe it should be inside remote desktop
    add_subdirectory (src/Services/MyRemoteDesktop/xwd-1.0.5/) 
    add_subdirectory (src/Services/MyRemoteDesktop)
    add_subdirectory (src/Services/GeoPosShare) 
    add_subdirectory (src/Services/SimpleTemplate) 
    add_subdirectory (src/Services/HabChan) 
    add_subdirectory (src/Services/CinemaPilot) 
    add_subdirectory (src/Services/SQLiteServer)
    
    #These are unit tests for the core library
    add_subdirectory (src/UnitTests)
   
# TODO FIX INSTALLATION DIRECTORIES 

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)
 

