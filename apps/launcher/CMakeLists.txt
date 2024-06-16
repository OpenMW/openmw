set(LAUNCHER
    datafilespage.cpp
    graphicspage.cpp
    sdlinit.cpp
    main.cpp
    maindialog.cpp
    textslotmsgbox.cpp
    importpage.cpp
    settingspage.cpp

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
    textslotmsgbox.hpp
    importpage.hpp
    settingspage.hpp

    utils/cellnameloader.hpp
    utils/profilescombobox.hpp
    utils/textinputdialog.hpp
    utils/lineedit.hpp
    utils/openalutil.hpp
)

# Headers that must be pre-processed
set(LAUNCHER_UI
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/datafilespage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/graphicspage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/importpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/settingspage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/directorypicker.ui
)

source_group(launcher FILES ${LAUNCHER} ${LAUNCHER_HEADER})

set(QT_USE_QTGUI 1)

set (LAUNCHER_RES ${CMAKE_SOURCE_DIR}/files/launcher/launcher.qrc)

# Set some platform specific settings
if(WIN32)
    set(LAUNCHER_RES ${LAUNCHER_RES} ${CMAKE_SOURCE_DIR}/files/windows/QWindowsVistaDark/dark.qrc)
    set(GUI_TYPE WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

QT_ADD_RESOURCES(RCC_SRCS ${LAUNCHER_RES})
QT_WRAP_UI(UI_HDRS ${LAUNCHER_UI})

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

add_dependencies(openmw-launcher qm-files)

if (WIN32)
    INSTALL(TARGETS openmw-launcher RUNTIME DESTINATION ".")
endif (WIN32)

target_link_libraries(openmw-launcher
    ${SDL2_LIBRARY_ONLY}
    ${OPENAL_LIBRARY}
    components_qt
)

target_link_libraries(openmw-launcher Qt::Widgets Qt::Core Qt::Svg)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-launcher PRIVATE --coverage)
    target_link_libraries(openmw-launcher gcov)
endif()

if(USE_QT)
    set_property(TARGET openmw-launcher PROPERTY AUTOMOC ON)
endif(USE_QT)

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw-launcher PRIVATE
        <boost/program_options/options_description.hpp>

        <algorithm>
        <string>
        <vector>
    )
endif()
