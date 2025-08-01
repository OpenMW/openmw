set(OSGQT_LIBRARY "osgQt")

# Sources

set(SOURCE_H
    osgQOpenGLWidget.hpp
    CompositeOsgRenderer.hpp
)

qt_wrap_cpp(SOURCES_H_MOC ${SOURCE_H} )

set(OSGQT_SOURCE_FILES
    osgQOpenGLWidget.cpp
    CompositeOsgRenderer.cpp
    ${SOURCES_H_MOC}
)

add_library(${OSGQT_LIBRARY} STATIC ${OSGQT_SOURCE_FILES})

target_link_libraries(${OSGQT_LIBRARY} Qt::Core Qt::OpenGL Qt::OpenGLWidgets)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EXTERN_OSGQT_LIBRARY ${OSGQT_LIBRARY} PARENT_SCOPE)
