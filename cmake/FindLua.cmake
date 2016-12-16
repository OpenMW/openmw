# - Try to find Lua
# Once done this will define
#
#  Lua_FOUND - system has Lua
#  Lua_INCLUDES - the Lua include directory
#  Lua_LIBRARY - Link these to use Lua

FIND_LIBRARY (Lua_LIBRARY_RELEASE NAMES lua5.1
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib
    $ENV{Lua_ROOT}/lib
    )
	
FIND_LIBRARY (Lua_LIBRARY_DEBUG NAMES lua5.1
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib
    $ENV{Lua_ROOT}/lib
    )	
	
	

FIND_PATH (Lua_INCLUDES lua5.1/lua.h
    ENV CPATH
    /usr/include
    /usr/local/include
    /opt/local/include
    $ENV{Lua_ROOT}/include
    )
 
IF(Lua_INCLUDES AND Lua_LIBRARY_RELEASE)
    SET(Lua_FOUND TRUE)
ENDIF(Lua_INCLUDES AND Lua_LIBRARY_RELEASE)

IF(NOT Lua_LIBRARY_DEBUG)
    SET(Lua_LIBRARY_DEBUG ${Lua_LIBRARY_RELEASE})
ENDIF()

IF(Lua_FOUND)
  SET(Lua_INCLUDES ${Lua_INCLUDES})

  find_package(ZLIB REQUIRED)
  
   IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
        SET(Lua_LIBRARY optimized ${Lua_LIBRARY_RELEASE} debug ${Lua_LIBRARY_DEBUG} ${ZLIB_LIBRARIES})
      ELSE()
        # if there are no configuration types and CMAKE_BUILD_TYPE has no value
        # then just use the release libraries
        SET(Lua_LIBRARY ${Lua_LIBRARY_RELEASE} ${ZLIB_LIBRARIES})
      ENDIF()
  IF(NOT Lua_FIND_QUIETLY)
    MESSAGE(STATUS "Found Lua: ${Lua_LIBRARIES}")
  ENDIF(NOT Lua_FIND_QUIETLY)
ELSE(Lua_FOUND)
  IF(Lua_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Lua")
  ENDIF(Lua_FIND_REQUIRED)
ENDIF(Lua_FOUND)

