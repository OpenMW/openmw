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
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} ${FFmpeg_LIBRARIES})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} ${OSG_LIBRARIES})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} SDL2::SDL2)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} PUBLIC
        <memory>
        <string>
        <vector>
    )

    target_precompile_headers(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} PRIVATE <algorithm>)
endif()
