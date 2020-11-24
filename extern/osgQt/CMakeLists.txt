set(OSGQT_LIBRARY "osgQt")

# Sources

set(SOURCE_H
        osgQOpenGLWidget
        osgQOpenGLWindow
        OSGRenderer
)

qt5_wrap_cpp(SOURCES_H_MOC ${SOURCE_H} )

set(OSGQT_SOURCE_FILES
    GraphicsWindowQt.cpp
    osgQOpenGLWidget.cpp
    osgQOpenGLWindow.cpp
    OSGRenderer.cpp
    ${SOURCES_H_MOC}
)

add_library(${OSGQT_LIBRARY} STATIC ${OSGQT_SOURCE_FILES})

if (DESIRED_QT_VERSION MATCHES 4)
    include(${QT_USE_FILE})
    target_link_libraries(${OSGQT_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTOPENGL_LIBRARY})
else()
    target_link_libraries(${OSGQT_LIBRARY} Qt5::Core Qt5::OpenGL Qt5::Widgets)
endif()

link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EXTERN_OSGQT_LIBRARY ${OSGQT_LIBRARY} PARENT_SCOPE)
