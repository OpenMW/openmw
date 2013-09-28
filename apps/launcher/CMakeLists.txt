set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    lineedit.cpp
    main.cpp
    maindialog.cpp
    naturalsort.cpp
    playpage.cpp
    pluginsmodel.cpp
    pluginsview.cpp
    filedialog.cpp

    launcher.rc
)

set(LAUNCHER_HEADER
    combobox.hpp
    datafilespage.hpp
    graphicspage.hpp
    lineedit.hpp
    maindialog.hpp
    naturalsort.hpp
    playpage.hpp
    pluginsmodel.hpp
    pluginsview.hpp
    filedialog.hpp
)

# Headers that must be pre-processed
set(LAUNCHER_HEADER_MOC
    combobox.hpp
    datafilespage.hpp
    graphicspage.hpp
    lineedit.hpp
    maindialog.hpp
    playpage.hpp
    pluginsmodel.hpp
    pluginsview.hpp
    filedialog.hpp
)

source_group(launcher FILES ${LAUNCHER} ${LAUNCHER_HEADER} ${LAUNCHER_HEADER_MOC})

find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI 1)

# Set some platform specific settings
if(WIN32)
    set(GUI_TYPE WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

QT4_ADD_RESOURCES(RCC_SRCS resources.qrc)
QT4_WRAP_CPP(MOC_SRCS ${LAUNCHER_HEADER_MOC})

include(${QT_USE_FILE})

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
    ${RCC_SRCS}
    ${MOC_SRCS}
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

if (APPLE)
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
        "${APP_BUNDLE_DIR}/../launcher.qss")
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.cfg
        "${APP_BUNDLE_DIR}/../launcher.cfg")
else()
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/launcher.qss")

    # Fallback in case getGlobalDataPath does not point to resources
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launcher.qss")

    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.cfg
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/launcher.cfg")
endif()
