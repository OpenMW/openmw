# - Try to find Terra
# Once done this will define
#
#  Terra_FOUND - system has Terra
#  Terra_INCLUDES - the Terra include directory
#  Terra_LIBRARY - Link these to use Terra

if(Win32)
SET(Terra_LIBRARY_Name terra.so)
else(Win32)
SET(Terra_LIBRARY_Name terra)
endif(Win32)

FIND_LIBRARY (Terra_LIBRARY_RELEASE NAMES ${Terra_LIBRARY_Name}
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib
    ${Terra_ROOT}/lib
    ${Terra_DIR}/lib
    )
	
FIND_LIBRARY (Terra_LIBRARY_DEBUG NAMES  ${Terra_LIBRARY_Name_Debug}
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib
    ${Terra_ROOT}/lib
    ${Terra_DIR}/lib
    )	
	
	

FIND_PATH (Terra_INCLUDES terra/terra.h
    ENV CPATH
    /usr/include
    /usr/local/include
    /opt/local/include
    ${Terra_ROOT}/include
    ${Terra_DIR}/include
    )
 
IF(Terra_INCLUDES AND Terra_LIBRARY_RELEASE)
    SET(Terra_FOUND TRUE)
ENDIF(Terra_INCLUDES AND Terra_LIBRARY_RELEASE)

IF(NOT Terra_LIBRARY_DEBUG)
    SET(Terra_LIBRARY_DEBUG ${Terra_LIBRARY_RELEASE})
ENDIF()

IF(Terra_FOUND)
  SET(Terra_INCLUDES ${Terra_INCLUDES})

  find_package(ZLIB REQUIRED)
  
   IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
        SET(Terra_LIBRARY optimized ${Terra_LIBRARY_RELEASE} debug ${Terra_LIBRARY_DEBUG} ${ZLIB_LIBRARIES} dl tinfo)
      ELSE()
        # if there are no configuration types and CMAKE_BUILD_TYPE has no value
        # then just use the release libraries
        SET(Terra_LIBRARY ${Terra_LIBRARY_RELEASE} ${ZLIB_LIBRARIES} dl tinfo)
      ENDIF()
  IF(NOT Terra_FIND_QUIETLY)
    MESSAGE(STATUS "Found Terra: ${Terra_LIBRARIES}")
  ENDIF(NOT Terra_FIND_QUIETLY)
ELSE(Terra_FOUND)
  IF(Terra_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Terra")
  ENDIF(Terra_FIND_REQUIRED)
ENDIF(Terra_FOUND)

