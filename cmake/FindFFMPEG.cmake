# Find the FFmpeg library
#
# Sets
#   FFMPEG_FOUND.  If false, don't try to use ffmpeg
#   FFMPEG_INCLUDE_DIR
#   FFMPEG_LIBRARIES
#
# Modified by Nicolay Korslund for OpenMW

SET( FFMPEG_FOUND "NO" )

FIND_PATH( FFMPEG_general_INCLUDE_DIR libavcodec/avcodec.h libavformat/avformat.h
  HINTS
  PATHS
  /usr/include
  /usr/local/include
  /usr/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg/libavcodec
  /usr/local/include/ffmpeg/libavcodec
  /usr/include/libavcodec
  /usr/local/include/libavcodec
  )

FIND_PATH( FFMPEG_avcodec_INCLUDE_DIR avcodec.h
  HINTS
  PATHS
  ${FFMPEG_general_INCLUDE_DIR}/libavcodec
  /usr/include
  /usr/local/include
  /usr/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg/libavcodec
  /usr/local/include/ffmpeg/libavcodec
  /usr/include/libavcodec
  /usr/local/include/libavcodec
)

FIND_PATH( FFMPEG_avformat_INCLUDE_DIR avformat.h
  HINTS
  PATHS
  ${FFMPEG_general_INCLUDE_DIR}/libavformat
  /usr/include
  /usr/local/include
  /usr/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg/libavformat
  /usr/local/include/ffmpeg/libavformat
  /usr/include/libavformat
  /usr/local/include/libavformat
)

set(FFMPEG_INCLUDE_DIR ${FFMPEG_general_INCLUDE_DIR} ${FFMPEG_avcodec_INCLUDE_DIR} ${FFMPEG_avformat_INCLUDE_DIR})

IF( FFMPEG_INCLUDE_DIR )

FIND_PROGRAM( FFMPEG_CONFIG ffmpeg-config
  /usr/bin
  /usr/local/bin
  ${HOME}/bin
)

IF( FFMPEG_CONFIG )
  EXEC_PROGRAM( ${FFMPEG_CONFIG} ARGS "--libs avformat" OUTPUT_VARIABLE FFMPEG_LIBS )
  SET( FFMPEG_FOUND "YES" )
  SET( FFMPEG_LIBRARIES "${FFMPEG_LIBS}" )
  
ELSE( FFMPEG_CONFIG )

  FIND_LIBRARY( FFMPEG_avcodec_LIBRARY avcodec
    /usr/lib
    /usr/local/lib
    /usr/lib64
    /usr/local/lib64
  )

  FIND_LIBRARY( FFMPEG_avformat_LIBRARY avformat
    /usr/lib
    /usr/local/lib
    /usr/lib64
    /usr/local/lib64
  )
  
  FIND_LIBRARY( FFMPEG_avutil_LIBRARY avutil
    /usr/lib
    /usr/local/lib
    /usr/lib64
    /usr/local/lib64
  )
  
  IF( FFMPEG_avcodec_LIBRARY )
  IF( FFMPEG_avformat_LIBRARY )

    SET( FFMPEG_FOUND "YES" )
    SET( FFMPEG_LIBRARIES ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} )
    IF( FFMPEG_avutil_LIBRARY )
       SET( FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} ${FFMPEG_avutil_LIBRARY} )
    ENDIF( FFMPEG_avutil_LIBRARY )

  ENDIF( FFMPEG_avformat_LIBRARY )
  ENDIF( FFMPEG_avcodec_LIBRARY )

ENDIF( FFMPEG_CONFIG )

ENDIF( FFMPEG_INCLUDE_DIR )
