set(OSG_FFMPEG_VIDEOPLAYER_LIBRARY "osg-ffmpeg-videoplayer")

# Sources

set(OSG_FFMPEG_VIDEOPLAYER_SOURCE_FILES
    videoplayer.cpp
    videostate.cpp
    audiodecoder.cpp
    include/osg-ffmpeg-videoplayer/audiodecoder.hpp
    include/osg-ffmpeg-videoplayer/audiofactory.hpp
    include/osg-ffmpeg-videoplayer/libavformatdefines.hpp
    include/osg-ffmpeg-videoplayer/libavutildefines.hpp
    include/osg-ffmpeg-videoplayer/videodefs.hpp
    include/osg-ffmpeg-videoplayer/videoplayer.hpp
    include/osg-ffmpeg-videoplayer/videostate.hpp
)

add_library(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} STATIC ${OSG_FFMPEG_VIDEOPLAYER_SOURCE_FILES})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} ${FFmpeg_LIBRARIES})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} ${OSG_LIBRARIES})
target_link_libraries(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} SDL2::SDL2)

target_include_directories(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} SYSTEM PUBLIC include ${FFmpeg_INCLUDE_DIRS})
target_include_directories(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} PRIVATE include/osg-ffmpeg-videoplayer)

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} PUBLIC
        <memory>
        <string>
        <vector>
    )

    target_precompile_headers(${OSG_FFMPEG_VIDEOPLAYER_LIBRARY} PRIVATE <algorithm>)
endif()
