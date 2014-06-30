set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    main.cpp
    maindialog.cpp
    playpage.cpp
    textslotmsgbox.cpp

    settings/gamesettings.cpp
    settings/graphicssettings.cpp
    settings/launchersettings.cpp

    utils/checkablemessagebox.cpp
    utils/profilescombobox.cpp
    utils/textinputdialog.cpp
    utils/lineedit.cpp

    ${CMAKE_SOURCE_DIR}/files/launcher/launcher.rc
)
if(NOT WIN32)
    LIST(APPEND LAUNCHER unshieldthread.cpp)
endif(NOT WIN32)

set(LAUNCHER_HEADER
    datafilespage.hpp
    graphicspage.hpp
    maindialog.hpp
    playpage.hpp
    textslotmsgbox.hpp

    settings/gamesettings.hpp
    settings/graphicssettings.hpp
    settings/launchersettings.hpp
    settings/settingsbase.hpp

    utils/checkablemessagebox.hpp
    utils/profilescombobox.hpp
    utils/textinputdialog.hpp
    utils/lineedit.hpp
)
if(NOT WIN32)
    LIST(APPEND LAUNCHER_HEADER unshieldthread.hpp)
endif(NOT WIN32)


# Headers that must be pre-processed
set(LAUNCHER_HEADER_MOC
    datafilespage.hpp
    graphicspage.hpp
    maindialog.hpp
    playpage.hpp
    textslotmsgbox.hpp

    utils/textinputdialog.hpp
    utils/checkablemessagebox.hpp
    utils/profilescombobox.hpp
    utils/lineedit.hpp

)

if(NOT WIN32)
    LIST(APPEND LAUNCHER_HEADER_MOC unshieldthread.hpp)
endif(NOT WIN32)


set(LAUNCHER_UI
    ${CMAKE_SOURCE_DIR}/files/ui/datafilespage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/graphicspage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/mainwindow.ui
    ${CMAKE_SOURCE_DIR}/files/ui/playpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/contentselector.ui
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
    include_directories(${LIBUNSHIELD_INCLUDE})
endif(NOT WIN32)

# Main executable
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
    ${SDL2_LIBRARY_ONLY}
    ${QT_LIBRARIES}
    components
)
if(NOT WIN32)
    target_link_libraries(omwlauncher
        ${LIBUNSHIELD_LIBRARY}
    )
endif(NOT WIN32)



if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(omwlauncher gcov)
endif()

