project(masterserver)

#set(CMAKE_CXX_STANDARD 14)
add_definitions(-std=gnu++14)

include_directories("./")

set(SOURCE_FILES main.cpp MasterServer.cpp MasterServer.hpp RestServer.cpp RestServer.hpp)

add_executable(masterserver ${SOURCE_FILES})
target_link_libraries(masterserver ${RakNet_LIBRARY} components)

option(BUILD_MASTER_TEST "build master server test program" OFF)

if(BUILD_MASTER_TEST)
    add_executable(ServerTest ServerTest.cpp)
    target_link_libraries(ServerTest ${RakNet_LIBRARY} components)
endif()

if (UNIX)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if(NOT APPLE)
        target_link_libraries(masterserver ${CMAKE_THREAD_LIBS_INIT})
        if(BUILD_MASTER_TEST)
            target_link_libraries(ServerTest ${CMAKE_THREAD_LIBS_INIT})
        endif()
    endif(NOT APPLE)
endif(UNIX)

if(WIN32) 
    target_link_libraries(masterserver wsock32) 
endif(WIN32)
