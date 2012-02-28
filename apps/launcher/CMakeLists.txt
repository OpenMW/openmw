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
)

source_group(launcher FILES ${LAUNCHER} ${LAUNCHER_HEADER} ${LAUNCHER_HEADER_MOC})

find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI 1)

if (NOT APPLE) # this dependency can be completely removed, but now it only tested on OS X
    find_package(PNG REQUIRED)
    include_directories(${PNG_INCLUDE_DIR})
endif()

QT4_ADD_RESOURCES(RCC_SRCS resources.qrc)
QT4_WRAP_CPP(MOC_SRCS ${LAUNCHER_HEADER_MOC})

include(${QT_USE_FILE})

# Main executable
add_executable(omwlauncher
    ${LAUNCHER}
    ${RCC_SRCS}
    ${MOC_SRCS}
)

target_link_libraries(omwlauncher
    ${Boost_LIBRARIES}
    ${OGRE_LIBRARIES}
    ${QT_LIBRARIES}
    ${PNG_LIBRARY}
    components
)

if(DPKG_PROGRAM)
    INSTALL(TARGETS omwlauncher RUNTIME DESTINATION games COMPONENT omwlauncher)
endif()

if (APPLE)
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
        "${APP_BUNDLE_DIR}/resources/../launcher.qss")
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
        "${APP_BUNDLE_DIR}/../launcher.cfg")
else()
    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.qss
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/launcher.qss")

    configure_file(${CMAKE_SOURCE_DIR}/files/launcher.cfg
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}launcher.cfg")
endif()
