set(OSGQT_LIBRARY "osgQt")

# Sources

set(SOURCE_H
    include/osgQt/osgQOpenGLWidget.hpp
    include/osgQt/CompositeOsgRenderer.hpp
)

qt_wrap_cpp(SOURCES_H_MOC ${SOURCE_H} )

set(OSGQT_SOURCE_FILES
    osgQOpenGLWidget.cpp
    CompositeOsgRenderer.cpp
    ${SOURCES_H_MOC}
)

add_library(${OSGQT_LIBRARY} STATIC ${OSGQT_SOURCE_FILES})
target_include_directories(${OSGQT_LIBRARY} PUBLIC include)
target_include_directories(${OSGQT_LIBRARY} PRIVATE include/osgQt)

target_link_libraries(${OSGQT_LIBRARY} Qt::Core Qt::OpenGL Qt::OpenGLWidgets)

set(EXTERN_OSGQT_LIBRARY ${OSGQT_LIBRARY} PARENT_SCOPE)
