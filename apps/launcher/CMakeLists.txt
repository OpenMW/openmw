set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    main.cpp
    maindialog.cpp
    playpage.cpp
    textslotmsgbox.cpp
    settingspage.cpp
    advancedpage.cpp

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
    advancedpage.hpp

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
    advancedpage.hpp

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
    ${CMAKE_SOURCE_DIR}/files/ui/advancedpage.ui
)

source_group(launcher FILES ${LAUNCHER} ${LAUNCHER_HEADER})

set(QT_USE_QTGUI 1)

# Set some platform specific settings
if(WIN32)
    set(GUI_TYPE WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

if (DESIRED_QT_VERSION MATCHES 4)
    include(${QT_USE_FILE})
    QT4_ADD_RESOURCES(RCC_SRCS ${CMAKE_SOURCE_DIR}/files/launcher/launcher.qrc)
    QT4_WRAP_CPP(MOC_SRCS ${LAUNCHER_HEADER_MOC})
    QT4_WRAP_UI(UI_HDRS ${LAUNCHER_UI})
else()
    QT5_ADD_RESOURCES(RCC_SRCS ${CMAKE_SOURCE_DIR}/files/launcher/launcher.qrc)
    QT5_WRAP_CPP(MOC_SRCS ${LAUNCHER_HEADER_MOC})
    QT5_WRAP_UI(UI_HDRS ${LAUNCHER_UI})
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
if(NOT WIN32)
    include_directories(${LIBUNSHIELD_INCLUDE_DIR})
endif(NOT WIN32)

# Main executable
openmw_add_executable(openmw-launcher
    ${GUI_TYPE}
    ${LAUNCHER}
    ${LAUNCHER_HEADER}
    ${RCC_SRCS}
    ${MOC_SRCS}
    ${UI_HDRS}
)

if (WIN32)
    INSTALL(TARGETS openmw-launcher RUNTIME DESTINATION ".")
endif (WIN32)

target_link_libraries(openmw-launcher
    ${SDL2_LIBRARY_ONLY}
    components
)

if (DESIRED_QT_VERSION MATCHES 4)
    target_link_libraries(openmw-launcher ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY})
    if(WIN32)
        target_link_libraries(openmw-launcher ${QT_QTMAIN_LIBRARY})
    endif(WIN32)
else()
    qt5_use_modules(openmw-launcher Widgets Core)
endif()

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-launcher gcov)
endif()


