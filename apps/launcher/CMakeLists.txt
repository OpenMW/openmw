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
)

if(WIN32 AND MSVC)
    # Use generated .rc file with absolute path to manifest
    list(APPEND LAUNCHER ${LAUNCHER_RC_FILE})
else()
    # For other platforms, use original .rc file
    list(APPEND LAUNCHER ${CMAKE_SOURCE_DIR}/files/windows/launcher.rc)
endif()

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

if(NOT WIN32)
    include_directories(${LIBUNSHIELD_INCLUDE_DIR})
endif(NOT WIN32)

# Main executable
openmw_add_executable(openmw-launcher
    ${GUI_TYPE}
    ${LAUNCHER}
    ${LAUNCHER_HEADER}
    ${RCC_SRCS}
)

add_dependencies(openmw-launcher qm-files)

if (WIN32)
    INSTALL(TARGETS openmw-launcher RUNTIME DESTINATION ".")
endif (WIN32)

target_link_libraries(openmw-launcher
    SDL2::SDL2
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
    set_property(TARGET openmw-launcher PROPERTY AUTOUIC ON)
    set_property(TARGET openmw-launcher PROPERTY AUTOUIC_SEARCH_PATHS ui)
endif(USE_QT)

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw-launcher PRIVATE
        <boost/program_options/options_description.hpp>

        <algorithm>
        <string>
        <vector>
    )
endif()
