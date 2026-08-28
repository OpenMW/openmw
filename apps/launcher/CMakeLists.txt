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
openmw_add_qt_executable(openmw-launcher
    ${GUI_TYPE}
    ${LAUNCHER}
    ${LAUNCHER_HEADER}
    ${RCC_SRCS}
)

add_dependencies(openmw-launcher qm-files)

if (NOT APPLE)
    install(TARGETS openmw-launcher RUNTIME_DEPENDENCY_SET openmw-dependencies)

    if (LINUX AND OPENMW_DEPLOY_RUNTIME_DEPENDENCIES)
        if (Qt6Core_VERSION VERSION_LESS 6.10)
            message(FATAL_ERROR "Deployment of Qt on Linux requires at least 6.10 (for the INCLUDE_PLUGINS argument)")
        endif ()
        # don't bother with this on Windows yet as windeployqt includes a bunch of DLLs we don't use
        if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
            cmake_path(NORMAL_PATH QT_DIR OUTPUT_VARIABLE QT_BASE_DIR)
        else ()
            # just hope it's close enough
        endif()
        string(REGEX REPLACE "(.+)/lib/cmake/Qt[0-9]+" "\\1" QT_BASE_DIR "${QT_BASE_DIR}")
        escape_regex_control_characters(QT_BASE_REGEX "${QT_BASE_DIR}")
        qt_generate_deploy_app_script(TARGET openmw-launcher
            OUTPUT_SCRIPT deploy_script
            NO_TRANSLATIONS
            NO_COMPILER_RUNTIME
            INCLUDE_PLUGINS wayland
            POST_INCLUDE_REGEXES
                "${QT_BASE_REGEX}"
            POST_EXCLUDE_REGEXES
                ".*"
        )
        install(SCRIPT ${deploy_script})
    endif()
endif ()

target_link_libraries(openmw-launcher PUBLIC
    SDL2::SDL2
    ${OPENAL_LIBRARY}
    components_qt
)

target_link_libraries(openmw-launcher PUBLIC Qt::Widgets Qt::Core Qt::Svg)

if (WIN32)
    target_sources(openmw-launcher PRIVATE ${CMAKE_SOURCE_DIR}/files/windows/other-apps.manifest)
endif()

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
