set(OSGQT_LIBRARY "osgQt")

# Sources

set(OSGQT_SOURCE_FILES
    GraphicsWindowQt.cpp
)

add_library(${OSGQT_LIBRARY} STATIC ${OSGQT_SOURCE_FILES})

target_link_libraries(${OSGQT_LIBRARY} Qt5::Core Qt5::OpenGL)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(EXTERN_OSGQT_LIBRARY ${OSGQT_LIBRARY} PARENT_SCOPE)
