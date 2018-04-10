project( AmmarServerProject ) 
cmake_minimum_required( VERSION 2.8.7 )

#We are a "serious" server application and need all the stack protection we can get..! 
set(CMAKE_CXX_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2")
set(CMAKE_C_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2")
 
#Use OpenSSL ( currently under construction )
OPTION (USE_OPENSSL OFF)
#Use LibEvent as a threading mechanism
OPTION (USE_LIBEVENT OFF)
#Use X-Server for applications that may need it ( MyRemoteDesktop )
OPTION (USE_XSERVER OFF)
#Flag to compile for RASPBERRYPI
OPTION (COMPILE_FOR_ARM OFF) 

#Compile Everything with Coverage test 
#https://en.wikipedia.org/wiki/Gcov
OPTION (DO_TEST_COVERAGE OFF)


#Use Amm Messages , if you are interested in generating custom messages through http you need this on
OPTION (USE_AMMMESSAGES OFF)


IF( DO_TEST_COVERAGE )
 MESSAGE("GCC will generate coverage information.. This will take a little longer") 
 MESSAGE("Compilation flags will be modified , please deactivate DO_TEST_COVERAGE if you want this to be your final build") 
ENDIF( DO_TEST_COVERAGE )

IF( USE_LIBEVENT )
 add_definitions(-DUSE_LIBEVENT)
ENDIF( USE_LIBEVENT )

IF( USE_OPENSSL )
 add_definitions(-DUSE_OPENSSL)
ENDIF( USE_OPENSSL )

IF( COMPILE_FOR_ARM )
 set(CMAKE_CXX_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2 -march=armv8-a")
 set(CMAKE_C_FLAGS "-fPIC -s -Wl,-z,relro,-z,now -fstack-protector -O2 -march=armv8-a")
ENDIF( COMPILE_FOR_ARM )

    #This is the core library
    add_subdirectory (src/AmmServerlib)
    add_subdirectory (src/AmmClient)
 
    #After we are done with the basics make all our secondary libraries
    add_subdirectory (src/AmmCaptcha)
    add_subdirectory (src/StringRecognizer)
    add_subdirectory (src/StringEscapeFile)
    add_subdirectory (src/FileToString)
    add_subdirectory (src/UserAccounts)
    add_subdirectory (src/Hashmap)
    add_subdirectory (src/InputParser)

IF (USE_AMMMESSAGES)
    add_subdirectory (src/AmmMessages)
ENDIF (USE_AMMMESSAGES)


    #Make the basic "vanilla" server  
    add_subdirectory (src/Services/AmmarServer) 

    #Then make all the showcase applications that are not very bad.. :P  
    add_subdirectory (src/Services/MyURL) 
    add_subdirectory (src/Services/MyLoader) 
    add_subdirectory (src/Services/MyBlog) 
    add_subdirectory (src/Services/MyTube)
    add_subdirectory (src/Services/MySearch)
    
    #Then make some little less well developed and more cringe-worthy applications 
    add_subdirectory (src/Services/GeoPosShare) 
    add_subdirectory (src/Services/Social)
    add_subdirectory (src/Services/HabChan) 
    add_subdirectory (src/Services/WebFramebuffer)
    add_subdirectory (src/Services/SimpleTemplate) 
    add_subdirectory (src/Services/V4L2ToHTTP)

    add_subdirectory (src/Services/AmmBus)

    
    IF( USE_XSERVER ) 
      #XWD is listed here maybe it should be inside remote desktop
      add_subdirectory (src/Services/MyRemoteDesktop/xwd-1.0.5/) 
      add_subdirectory (src/Services/MyRemoteDesktop)
    ENDIF( USE_XSERVER )
    
    #Various utilities for specific use-cases
    add_subdirectory (src/Services/Various/NaoNetWalk)
    add_subdirectory (src/Services/Various/ScriptRunner) 

    #Deprecated utilities that will be removed in the future..
    add_subdirectory (src/Services/Deprecated/CinemaPilot) 
    
    #These are unit tests for the core library
    IF( DO_TEST_COVERAGE )   
     add_subdirectory (src/UnitTests/UserAccountsTester)
     add_subdirectory (src/UnitTests/HashMapTester)
    ENDIF( DO_TEST_COVERAGE )


# TODO FIX INSTALLATION DIRECTORIES 

#    install(TARGETS AmmarServer
#                    LIBRARY DESTINATION lib
#                    ARCHIVE DESTINATION lib
#                    RUNTIME DESTINATION bin)

