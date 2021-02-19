set(OSG_FFMPEG_VIDEOPLAYER_LIBRARY "osg-ffmpeg-videoplayer")

# Sources

set(OSG_FFMPEG_VIDEOPLAYER_SOURCE_FILES
    videoplayer.cpp
    videostate.cpp
    videodefs.hpp
    audiodecoder.cpp
    audiofactory.hpp
)

include_directories(${FFmpeg_INCLUDE_DIRS})
add_library(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} STATIC ${OSG_FFMPEG_VIDEOPLAYER_SOURCE_FILES})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} ${FFmpeg_LIBRARIES} ${Boost_THREAD_LIBRARY})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} ${OSG_LIBRARIES})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
