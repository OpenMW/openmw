set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    sdlinit.cpp
    main.cpp
    maindialog.cpp
    playpage.cpp
    textslotmsgbox.cpp
    settingspage.cpp
    advancedpage.cpp

    utils/cellnameloader.cpp
    utils/profilescombobox.cpp
    utils/textinputdialog.cpp
    utils/lineedit.cpp
    utils/openalutil.cpp

    ${CMAKE_SOURCE_DIR}/files/windows/launcher.rc
)

set(LAUNCHER_HEADER
    datafilespage.hpp
    graphicspage.hpp
    sdlinit.hpp
    maindialog.hpp
    playpage.hpp
    textslotmsgbox.hpp
    settingspage.hpp
    advancedpage.hpp

    utils/cellnameloader.hpp
    utils/profilescombobox.hpp
    utils/textinputdialog.hpp
    utils/lineedit.hpp
    utils/openalutil.hpp
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

    utils/cellnameloader.hpp
    utils/textinputdialog.hpp
    utils/profilescombobox.hpp
    utils/lineedit.hpp
    utils/openalutil.hpp

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

QT5_ADD_RESOURCES(RCC_SRCS ${CMAKE_SOURCE_DIR}/files/launcher/launcher.qrc)
QT5_WRAP_CPP(MOC_SRCS ${LAUNCHER_HEADER_MOC})
QT5_WRAP_UI(UI_HDRS ${LAUNCHER_UI})

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
    ${OPENAL_LIBRARY}
    components
)

target_link_libraries(openmw-launcher Qt5::Widgets Qt5::Core)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-launcher gcov)
endif()


