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

# Find FFMPEG
set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE SWRESAMPLE AVRESAMPLE)
unset(FFMPEG_LIBRARIES CACHE)
find_package(FFmpeg)
if ( NOT AVCODEC_FOUND OR NOT AVFORMAT_FOUND OR NOT AVUTIL_FOUND OR NOT SWSCALE_FOUND )
    message(FATAL_ERROR "FFmpeg component required, but not found!")
endif()
set(VIDEO_FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES})
if( SWRESAMPLE_FOUND )
    add_definitions(-DHAVE_LIBSWRESAMPLE)
    set(VIDEO_FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${SWRESAMPLE_LIBRARIES})
else()
    if( AVRESAMPLE_FOUND )
        set(VIDEO_FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${AVRESAMPLE_LIBRARIES})
    else()
        message(FATAL_ERROR "Install either libswresample (FFmpeg) or libavresample (Libav).")
    endif()
endif()
include_directories(${FFMPEG_INCLUDE_DIRS})

# Find Boost
set(BOOST_COMPONENTS thread)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(${OGRE_FFMPEG_VIDEOPLAYER_LIBRARY} STATIC ${OGRE_FFMPEG_VIDEOPLAYER_SOURCE_FILES})
target_link_libraries(${OGRE_FFMPEG_VIDEOPLAYER_LIBRARY} ${VIDEO_FFMPEG_LIBRARIES} ${Boost_LIBRARIES})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
