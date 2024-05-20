set(WIZARD
    componentselectionpage.cpp
    conclusionpage.cpp
    existinginstallationpage.cpp
    importpage.cpp
    inisettings.cpp
    installationtargetpage.cpp
    intropage.cpp
    languageselectionpage.cpp
    main.cpp
    mainwizard.cpp
    methodselectionpage.cpp

    utils/componentlistwidget.cpp
)

if(WIN32)
    list(APPEND WIZARD ${CMAKE_SOURCE_DIR}/files/windows/openmw-wizard.rc)
endif()

set(WIZARD_HEADER
    componentselectionpage.hpp
    conclusionpage.hpp
    existinginstallationpage.hpp
    importpage.hpp
    inisettings.hpp
    installationtargetpage.hpp
    intropage.hpp
    languageselectionpage.hpp
    mainwizard.hpp
    methodselectionpage.hpp

    utils/componentlistwidget.hpp
)

set(WIZARD_UI
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/componentselectionpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/conclusionpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/existinginstallationpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/importpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/installationtargetpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/intropage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/languageselectionpage.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/methodselectionpage.ui
)

if (OPENMW_USE_UNSHIELD)
    set (WIZARD ${WIZARD} installationpage.cpp unshield/unshieldworker.cpp)
    set (WIZARD_HEADER ${WIZARD_HEADER} installationpage.hpp unshield/unshieldworker.hpp)
    set (WIZARD_UI ${WIZARD_UI} ${CMAKE_CURRENT_SOURCE_DIR}/ui/installationpage.ui)
    add_definitions(-DOPENMW_USE_UNSHIELD)
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
QT_WRAP_UI(UI_HDRS ${WIZARD_UI})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if (OPENMW_USE_UNSHIELD)
    include_directories(${LIBUNSHIELD_INCLUDE_DIRS})
endif()

openmw_add_executable(openmw-wizard
    ${GUI_TYPE}
    ${WIZARD}
    ${WIZARD_HEADER}
    ${RCC_SRCS}
    ${UI_HDRS}
)

add_dependencies(openmw-wizard qm-files)

target_link_libraries(openmw-wizard
    components_qt
)

target_link_libraries(openmw-wizard Qt::Widgets Qt::Core Qt::Svg)

if (OPENMW_USE_UNSHIELD)
    target_link_libraries(openmw-wizard ${LIBUNSHIELD_LIBRARIES})
endif()

if(DPKG_PROGRAM)
    INSTALL(TARGETS openmw-wizard RUNTIME DESTINATION games COMPONENT openmw-wizard)
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-wizard PRIVATE --coverage)
    target_link_libraries(openmw-wizard gcov)
endif()

# Workaround for binutil => 2.23 problem when linking, should be fixed eventually upstream
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(openmw-wizard dl Xt)
endif()

if (WIN32)
    INSTALL(TARGETS openmw-wizard RUNTIME DESTINATION ".")
endif(WIN32)

if(USE_QT)
    set_property(TARGET openmw-wizard PROPERTY AUTOMOC ON)
endif(USE_QT)
