set(WIZARD
    componentselectionpage.cpp
    conclusionpage.cpp
    existinginstallationpage.cpp
    importpage.cpp
    inisettings.cpp
    installationpage.cpp
    installationtargetpage.cpp
    intropage.cpp
    languageselectionpage.cpp
    main.cpp
    mainwizard.cpp
    methodselectionpage.cpp

    unshield/unshieldworker.cpp

    utils/componentlistwidget.cpp
)

set(WIZARD_HEADER
    componentselectionpage.hpp
    conclusionpage.hpp
    existinginstallationpage.hpp
    importpage.hpp
    inisettings.hpp
    installationpage.hpp
    installationtargetpage.hpp
    intropage.hpp
    languageselectionpage.hpp
    mainwizard.hpp
    methodselectionpage.hpp

    unshield/unshieldworker.hpp

    utils/componentlistwidget.hpp
)

# Headers that must be pre-processed
set(WIZARD_HEADER_MOC
    componentselectionpage.hpp
    conclusionpage.hpp
    existinginstallationpage.hpp
    importpage.hpp
    installationpage.hpp
    installationtargetpage.hpp
    intropage.hpp
    languageselectionpage.hpp
    mainwizard.hpp
    methodselectionpage.hpp

    unshield/unshieldworker.hpp

    utils/componentlistwidget.hpp
)

set(WIZARD_UI
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/componentselectionpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/conclusionpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/existinginstallationpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/importpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/installationpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/installationtargetpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/intropage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/languageselectionpage.ui
    ${CMAKE_SOURCE_DIR}/files/ui/wizard/methodselectionpage.ui
)

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
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LIBUNSHIELD_INCLUDE})

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

if(NOT WIN32)
    target_link_libraries(openmw-wizard
        ${LIBUNSHIELD_LIBRARY}
    )
endif(NOT WIN32)

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

