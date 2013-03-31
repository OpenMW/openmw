set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    main.cpp
    maindialog.cpp
    playpage.cpp

    settings/gamesettings.cpp
    settings/graphicssettings.cpp
    settings/launchersettings.cpp

    utils/checkablemessagebox.cpp
    utils/textinputdialog.cpp

    ${CMAKE_SOURCE_DIR}/files/launcher/launcher.rc
)

set(LAUNCHER_HEADER
    datafilespage.hpp
    graphicspage.hpp
    maindialog.hpp
    playpage.hpp

    settings/gamesettings.hpp
    settings/graphicssettings.hpp
    settings/launchersettings.hpp
    settings/settingsbase.hpp

    utils/checkablemessagebox.hpp
    utils/textinputdialog.hpp

)

# Headers that must be pre-processed
set(LAUNCHER_HEADER_MOC
    datafilespage.hpp
    graphicspage.hpp
    maindialog.hpp
    playpage.hpp

    utils/checkablemessagebox.hpp
    utils/textinputdialog.hpp
)

set(LAUNCHER_UI
    ${CMAKE_SOURCE_DIR}/files/ui/datafilespage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/graphicspage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/mainwindow.ui
    ${CMAKE_SOURCE_DIR}/files/ui/playpage.ui
)

source_group(launcher FILES ${LAUNCHER} ${LAUNCHER_HEADER})

find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI 1)

# Set some platform specific settings
if(WIN32)
    set(GUI_TYPE WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

QT4_ADD_RESOURCES(RCC_SRCS ${CMAKE_SOURCE_DIR}/files/launcher/launcher.qrc)
QT4_WRAP_CPP(MOC_SRCS ${LAUNCHER_HEADER_MOC})
QT4_WRAP_UI(UI_HDRS ${LAUNCHER_UI})

include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Main executable
IF(OGRE_STATIC)
IF(WIN32)
ADD_DEFINITIONS(-DENABLE_PLUGIN_Direct3D9 -DENABLE_PLUGIN_GL)
set(OGRE_STATIC_PLUGINS ${OGRE_RenderSystem_Direct3D9_LIBRARIES} ${OGRE_RenderSystem_GL_LIBRARIES})
ELSE(WIN32)
ADD_DEFINITIONS(-DENABLE_PLUGIN_GL)
set(OGRE_STATIC_PLUGINS ${OGRE_RenderSystem_GL_LIBRARIES})
ENDIF(WIN32)
ENDIF(OGRE_STATIC)
add_executable(omwlauncher
    ${GUI_TYPE}
    ${LAUNCHER}
    ${LAUNCHER_HEADER}
    ${RCC_SRCS}
    ${MOC_SRCS}
    ${UI_HDRS}
)

target_link_libraries(omwlauncher
    ${Boost_LIBRARIES}
    ${OGRE_LIBRARIES}
    ${OGRE_STATIC_PLUGINS}
    ${QT_LIBRARIES}
    components
)

if(DPKG_PROGRAM)
    INSTALL(TARGETS omwlauncher RUNTIME DESTINATION games COMPONENT omwlauncher)
endif()

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(omwlauncher gcov)
endif()
