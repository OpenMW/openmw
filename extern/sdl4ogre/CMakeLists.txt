set(SDL4OGRE_LIBRARY "sdl4ogre")

# Sources

set(SDL4OGRE_SOURCE_FILES
	sdlinputwrapper.cpp
	sdlcursormanager.cpp
	sdlwindowhelper.cpp
	imagerotate.cpp
)

if (APPLE)
    set(SDL4OGRE_SOURCE_FILES ${SDL4OGRE_SOURCE_FILES} osx_utils.mm)
endif ()

set(SDL4OGRE_HEADER_FILES
	OISCompat.h
	cursormanager.hpp
	events.h
)

add_library(${SDL4OGRE_LIBRARY} STATIC ${SDL4OGRE_SOURCE_FILES} ${SDL4OGRE_HEADER_FILES})

link_directories(${CMAKE_CURRENT_BINARY_DIR})


target_link_libraries(${SDL4OGRE_LIBRARY} ${SDL2_LIBRARY})
