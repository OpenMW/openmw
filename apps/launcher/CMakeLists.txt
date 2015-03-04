set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    main.cpp
    maindialog.cpp
    playpage.cpp
    textslotmsgbox.cpp
    settingspage.cpp

    settings/graphicssettings.cpp

    utils/profilescombobox.cpp
    utils/textinputdialog.cpp
    utils/lineedit.cpp

    ${CMAKE_SOURCE_DIR}/files/windows/launcher.rc
)

set(LAUNCHER_HEADER
    datafilespage.hpp
    graphicspage.hpp
    maindialog.hpp
    playpage.hpp
    textslotmsgbox.hpp
    settingspage.hpp

    settings/graphicssettings.hpp

    utils/profilescombobox.hpp
    utils/textinputdialog.hpp
    utils/lineedit.hpp
)

# Headers that must be pre-processed
set(LAUNCHER_HEADER_MOC
    datafilespage.hpp
    graphicspage.hpp
    maindialog.hpp
    playpage.hpp
    textslotmsgbox.hpp
    settingspage.hpp

    utils/textinputdialog.hpp
    utils/profilescombobox.hpp
    utils/lineedit.hpp

)

set(LAUNCHER_UI
    ${CMAKE_SOURCE_DIR}/files/ui/datafilespage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/graphicspage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/mainwindow.ui
    ${CMAKE_SOURCE_DIR}/files/ui/playpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/contentselector.ui
    ${CMAKE_SOURCE_DIR}/files/ui/settingspage.ui
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
if(NOT WIN32)
    include_directories(${LIBUNSHIELD_INCLUDE_DIR})
endif(NOT WIN32)

# Main executable
add_executable(openmw-launcher
    ${GUI_TYPE}
    ${LAUNCHER}
    ${LAUNCHER_HEADER}
    ${RCC_SRCS}
    ${MOC_SRCS}
    ${UI_HDRS}
)

target_link_libraries(openmw-launcher
    ${Boost_LIBRARIES}
    ${OGRE_LIBRARIES}
    ${OGRE_STATIC_PLUGINS}
    ${SDL2_LIBRARY_ONLY}
    ${QT_LIBRARIES}
    components
)


if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-launcher gcov)
endif()

