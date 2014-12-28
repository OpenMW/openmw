if (WIN32) # windows users can just run the morrowind installer
    set(OPENMW_USE_UNSHIELD FALSE)
else()
    set(OPENMW_USE_UNSHIELD TRUE)

    find_package(LIBUNSHIELD REQUIRED)
    if(NOT LIBUNSHIELD_FOUND)
        message(FATAL_ERROR "Failed to find Unshield library")
    endif(NOT LIBUNSHIELD_FOUND)
endif()

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

# Headers that must be pre-processed
set(WIZARD_HEADER_MOC
    componentselectionpage.hpp
    conclusionpage.hpp
    existinginstallationpage.hpp
    importpage.hpp
    installationtargetpage.hpp
    intropage.hpp
    languageselectionpage.hpp
    mainwizard.hpp
    methodselectionpage.hpp

    utils/componentlistwidget.hpp
)

set(WIZARD_UI
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/componentselectionpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/conclusionpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/existinginstallationpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/importpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/installationtargetpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/intropage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/languageselectionpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/methodselectionpage.ui
)

if (OPENMW_USE_UNSHIELD)
    set (WIZARD ${WIZARD} installationpage.cpp unshield/unshieldworker.cpp)
    set (WIZARD_HEADER ${WIZARD_HEADER} installationpage.hpp unshield/unshieldworker.hpp)
    set (WIZARD_HEADER_MOC ${WIZARD_HEADER_MOC} installationpage.hpp unshield/unshieldworker.hpp)
    set (WIZARD_UI ${WIZARD_UI} ${CMAKE_SOURCE_DIR}/files/ui/wizard/installationpage.ui)
    add_definitions(-DOPENMW_USE_UNSHIELD)
endif (OPENMW_USE_UNSHIELD)


source_group(wizard FILES ${WIZARD} ${WIZARD_HEADER})

find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI 1)

# Set some platform specific settings
if(WIN32)
    set(GUI_TYPE WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

QT4_ADD_RESOURCES(RCC_SRCS ${CMAKE_SOURCE_DIR}/files/wizard/wizard.qrc)
QT4_WRAP_CPP(MOC_SRCS ${WIZARD_HEADER_MOC})
QT4_WRAP_UI(UI_HDRS ${WIZARD_UI})


include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if (OPENMW_USE_UNSHIELD)
    include_directories(${LIBUNSHIELD_INCLUDE_DIR})
endif()

add_executable(openmw-wizard
    ${GUI_TYPE}
    ${WIZARD}
    ${WIZARD_HEADER}
    ${RCC_SRCS}
    ${MOC_SRCS}
    ${UI_HDRS}
)

target_link_libraries(openmw-wizard
    ${Boost_LIBRARIES}
    ${QT_LIBRARIES}
    components
)

if (OPENMW_USE_UNSHIELD)
    target_link_libraries(openmw-wizard ${LIBUNSHIELD_LIBRARY})
endif()


if(DPKG_PROGRAM)
    INSTALL(TARGETS openmw-wizard RUNTIME DESTINATION games COMPONENT openmw-wizard)
endif()

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-wizard gcov)
endif()

# Workaround for binutil => 2.23 problem when linking, should be fixed eventually upstream
if (UNIX AND NOT APPLE)
target_link_libraries(openmw-wizard dl Xt)
endif()

