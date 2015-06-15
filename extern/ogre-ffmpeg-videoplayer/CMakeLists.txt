set(OGRE_FFMPEG_VIDEOPLAYER_LIBRARY "ogre-ffmpeg-videoplayer")

# Sources

set(OGRE_FFMPEG_VIDEOPLAYER_SOURCE_FILES
    videoplayer.cpp
    videostate.cpp
    videodefs.hpp
    libavwrapper.cpp
    audiodecoder.cpp
    audiofactory.hpp
)

include_directories(${FFMPEG_INCLUDE_DIRS})
add_library(${OGRE_FFMPEG_VIDEOPLAYER_LIBRARY} STATIC ${OGRE_FFMPEG_VIDEOPLAYER_SOURCE_FILES})
target_link_libraries(${OGRE_FFMPEG_VIDEOPLAYER_LIBRARY} ${FFMPEG_LIBRARIES} ${Boost_THREAD_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
