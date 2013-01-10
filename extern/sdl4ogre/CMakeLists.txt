set(SDL4OGRE_LIBRARY "sdl4ogre")

# Sources

set(SDL4OGRE_SOURCE_FILES
	sdlinputwrapper.cpp
)

add_library(${SDL4OGRE_LIBRARY} STATIC ${SDL4OGRE_SOURCE_FILES})

link_directories(${CMAKE_CURRENT_BINARY_DIR})


target_link_libraries(${SDL4OGRE_LIBRARY} ${SDL2_LIBRARY})
