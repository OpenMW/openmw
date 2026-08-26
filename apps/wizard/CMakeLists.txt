set(WIZARD
    componentselectionpage.cpp
    conclusionpage.cpp
    existinginstallationpage.cpp
    importpage.cpp
    installationtargetpage.cpp
    intropage.cpp
    languageselectionpage.cpp
    main.cpp
    mainwizard.cpp
    methodselectionpage.cpp
)

if(WIN32)
    list(APPEND WIZARD ${CMAKE_SOURCE_DIR}/files/windows/openmw-wizard.rc)
endif()

set(WIZARD_HEADER
    componentselectionpage.hpp
    conclusionpage.hpp
    existinginstallationpage.hpp
    importpage.hpp
    installationtargetpage.hpp
    intropage.hpp
    languageselectionpage.hpp
    mainwizard.hpp
    methodselectionpage.hpp
)

if (OPENMW_USE_UNSHIELD)
    set (WIZARD
         ${WIZARD}
         installationpage.cpp
         unshield/inisettings.cpp
         unshield/unshieldworker.cpp
    )
    set (WIZARD_HEADER
         ${WIZARD_HEADER}
         installationpage.hpp
         unshield/inisettings.hpp
         unshield/unshieldworker.hpp
    )
    add_compile_definitions(OPENMW_USE_UNSHIELD)
endif (OPENMW_USE_UNSHIELD)


source_group(wizard FILES ${WIZARD} ${WIZARD_HEADER})

set(QT_USE_QTGUI 1)

set (WIZARD_RES ${CMAKE_SOURCE_DIR}/files/wizard/wizard.qrc)

# Set some platform specific settings
if(WIN32)
    set(WIZARD_RES ${WIZARD_RES} ${CMAKE_SOURCE_DIR}/files/windows/QWindowsVistaDark/dark.qrc)
    set(GUI_TYPE WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

QT_ADD_RESOURCES(RCC_SRCS ${WIZARD_RES})

if (OPENMW_USE_UNSHIELD)
    include_directories(${LIBUNSHIELD_INCLUDE_DIRS})
endif()

openmw_add_qt_executable(openmw-wizard
    ${GUI_TYPE}
    ${WIZARD}
    ${WIZARD_HEADER}
    ${RCC_SRCS}
)

add_dependencies(openmw-wizard qm-files)

target_link_libraries(openmw-wizard PRIVATE
    components_qt
)

target_link_libraries(openmw-wizard PRIVATE Qt::Widgets Qt::Core Qt::Svg)

if (OPENMW_USE_UNSHIELD)
    target_link_libraries(openmw-wizard PRIVATE ${LIBUNSHIELD_LIBRARIES})
endif()

if (WIN32)
    target_sources(openmw-wizard PRIVATE ${CMAKE_SOURCE_DIR}/files/windows/other-apps.manifest)
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-wizard PRIVATE --coverage)
    target_link_libraries(openmw-wizard PRIVATE gcov)
endif()

if (NOT APPLE)
    install(TARGETS openmw-wizard RUNTIME_DEPENDENCY_SET openmw-dependencies)

    if (LINUX AND OPENMW_DEPLOY_RUNTIME_DEPENDENCIES)
        if (Qt6Core_VERSION VERSION_LESS 6.10)
            message(FATAL_ERROR "Deployment of Qt on Linux requires at least 6.10 (for the INCLUDE_PLUGINS argument)")
        endif ()
        # don't bother with this on Windows yet as windeployqt includes a bunch of DLLs we don't use
        cmake_path(NORMAL_PATH QT_DIR OUTPUT_VARIABLE QT_BASE_DIR)
        string(REGEX REPLACE "(.+)/lib/cmake/Qt[0-9]+" "\\1" QT_BASE_DIR "${QT_BASE_DIR}")
        escape_regex_control_characters(QT_BASE_REGEX "${QT_BASE_DIR}")
        qt_generate_deploy_app_script(TARGET openmw-wizard
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
endif()

if(USE_QT)
    set_property(TARGET openmw-wizard PROPERTY AUTOMOC ON)
    set_property(TARGET openmw-wizard PROPERTY AUTOUIC ON)
    set_property(TARGET openmw-wizard PROPERTY AUTOUIC_SEARCH_PATHS ui)
endif(USE_QT)
