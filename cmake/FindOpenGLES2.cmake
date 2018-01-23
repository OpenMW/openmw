#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OpenGLES and EGL
# Once done this will define
#
#  OPENGLES2_FOUND        - system has OpenGLES
#  OPENGLES2_INCLUDE_DIR  - the GL include directory
#  OPENGLES2_LIBRARIES    - Link these to use OpenGLES
#
#  EGL_FOUND        - system has EGL
#  EGL_INCLUDE_DIR  - the EGL include directory
#  EGL_LIBRARIES    - Link these to use EGL

IF (WIN32)
    IF (CYGWIN)

        FIND_PATH(OPENGLES2_INCLUDE_DIR GLES2/gl2.h )

        FIND_LIBRARY(OPENGLES2_gl_LIBRARY libGLESv2 )

    ELSE (CYGWIN)

        IF(BORLAND)
            SET (OPENGLES2_gl_LIBRARY import32 CACHE STRING "OpenGL ES 2.x library for win32")
        ELSE(BORLAND)
            #MS compiler - todo - fix the following line:
            SET (OPENGLES2_gl_LIBRARY ${OGRE_DEPENDENCIES_DIR}/lib/release/libGLESv2.lib CACHE STRING "OpenGL ES 2.x library for win32")
            SET (EGL_egl_LIBRARY      ${OGRE_DEPENDENCIES_DIR}/lib/release/libEGL.lib CACHE STRING "EGL library for win32")
        ENDIF(BORLAND)

    ENDIF (CYGWIN)

ELSE (WIN32)

    IF (APPLE)

        create_search_paths(/Developer/Platforms)
        findpkg_framework(OpenGLES2)
        set(OPENGLES2_gl_LIBRARY "-framework OpenGLES")

    ELSE(APPLE)

        FIND_PATH(OPENGLES2_INCLUDE_DIR GLES2/gl2.h
                /usr/openwin/share/include
                /opt/graphics/OpenGL/include /usr/X11R6/include
                /usr/include
                )

        FIND_LIBRARY(OPENGLES2_gl_LIBRARY
                NAMES GLESv2
                PATHS /opt/graphics/OpenGL/lib
                /usr/openwin/lib
                /usr/shlib /usr/X11R6/lib
                /usr/lib
                )

        FIND_PATH(EGL_INCLUDE_DIR EGL/egl.h
                /usr/openwin/share/include
                /opt/graphics/OpenGL/include /usr/X11R6/include
                /usr/include
                )

        FIND_LIBRARY(EGL_egl_LIBRARY
                NAMES EGL
                PATHS /opt/graphics/OpenGL/lib
                /usr/openwin/lib
                /usr/shlib /usr/X11R6/lib
                /usr/lib
                )

        # On Unix OpenGL most certainly always requires X11.
        # Feel free to tighten up these conditions if you don't
        # think this is always true.
        # It's not true on OSX.

        IF (OPENGLES2_gl_LIBRARY)
            IF(NOT X11_FOUND)
                INCLUDE(FindX11)
            ENDIF(NOT X11_FOUND)
            IF (X11_FOUND)
                IF (NOT APPLE)
                    SET (OPENGLES2_LIBRARIES ${X11_LIBRARIES})
                ENDIF (NOT APPLE)
            ENDIF (X11_FOUND)
        ENDIF (OPENGLES2_gl_LIBRARY)

    ENDIF(APPLE)
ENDIF (WIN32)

SET( OPENGLES2_FOUND "YES" )
IF(OPENGLES2_gl_LIBRARY AND EGL_egl_LIBRARY)

    SET( OPENGLES2_LIBRARIES ${OPENGLES2_gl_LIBRARY} ${OPENGLES2_LIBRARIES})
    SET( EGL_LIBRARIES ${EGL_egl_LIBRARY} ${EGL_LIBRARIES})
    SET( OPENGLES2_FOUND "YES" )

ENDIF(OPENGLES2_gl_LIBRARY AND EGL_egl_LIBRARY)

MARK_AS_ADVANCED(
        OPENGLES2_INCLUDE_DIR
        OPENGLES2_gl_LIBRARY
        EGL_INCLUDE_DIR
        EGL_egl_LIBRARY
)