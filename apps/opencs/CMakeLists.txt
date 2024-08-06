set (OPENCS_SRC
    )

opencs_units (. editor)

opencs_units (model/doc
    document operation saving documentmanager loader runner operationholder
    )

opencs_units (model/doc
    savingstate savingstages blacklist messages
    )

opencs_hdrs (model/doc
    state
    )


opencs_units (model/world
    idtable idtableproxymodel regionmap data commanddispatcher idtablebase resourcetable nestedtableproxymodel idtree infotableproxymodel landtexturetableproxymodel
    actoradapter idcollection
    )


opencs_units (model/world
    universalid record commands columnbase columnimp scriptcontext cell refidcollection
    refiddata refidadapterimp ref collectionbase refcollection columns infocollection tablemimedata cellcoordinates cellselection resources resourcesmanager scope
    pathgrid land nestedtablewrapper nestedcollection nestedcoladapterimp nestedinfocollection
    idcompletionmanager metadata defaultgmsts infoselectwrapper commandmacro
    )

opencs_hdrs (model/world
    columnimp disabletag idcollection collection info subcellcollection
    )


opencs_units (model/tools
    tools reportmodel mergeoperation
    )

opencs_units (model/tools
    mandatoryid skillcheck classcheck factioncheck racecheck soundcheck regioncheck
    birthsigncheck spellcheck referencecheck referenceablecheck scriptcheck bodypartcheck
    startscriptcheck search searchoperation searchstage pathgridcheck soundgencheck magiceffectcheck
    mergestages gmstcheck topicinfocheck journalcheck enchantmentcheck effectlistcheck
    )

opencs_hdrs (model/tools
    mergestate
    )


opencs_units (view/doc
    viewmanager view operations operation subview startup filedialog newgame
    filewidget adjusterwidget loader globaldebugprofilemenu runlogsubview sizehint
    )


opencs_units (view/doc
    subviewfactory
    )

opencs_hdrs (view/doc
    subviewfactoryimp
    )


opencs_units (view/world
    table tablesubview scriptsubview util regionmapsubview tablebottombox creator genericcreator globalcreator
    cellcreator pathgridcreator referenceablecreator startscriptcreator referencecreator scenesubview
    infocreator scriptedit dialoguesubview previewsubview regionmap dragrecordtable nestedtable
    dialoguespinbox recordbuttonbar tableeditidaction scripterrortable extendedcommandconfigurator
    bodypartcreator landcreator tableheadermouseeventhandler
    )

opencs_units (view/world
    subviews enumdelegate vartypedelegate recordstatusdelegate idtypedelegate datadisplaydelegate
    scripthighlighter idvalidator dialoguecreator idcompletiondelegate
    colordelegate dragdroputils
    )

opencs_units (view/widget
    scenetoolbar scenetool scenetoolmode pushbutton scenetooltoggle scenetoolrun modebutton
    scenetooltoggle2 scenetooltexturebrush scenetoolshapebrush completerpopup coloreditor colorpickerpopup droplineedit
    )

opencs_units (view/render
    scenewidget worldspacewidget pagedworldspacewidget unpagedworldspacewidget
    previewwidget editmode instancemode instanceselectionmode instancemovemode
    orbitcameramode pathgridmode selectionmode pathgridselectionmode cameracontroller
    cellwater terraintexturemode actor terrainselection terrainshapemode brushdraw commands
    )

opencs_units (view/render
    lighting lightingday lightingnight lightingbright object cell terrainstorage tagbase
    cellarrow cellmarker cellborder pathgrid
    )

opencs_hdrs (view/render
    mask
    )


opencs_units (view/tools
    reportsubview reporttable searchsubview searchbox merge
    )

opencs_units (view/tools
    subviews
    )

opencs_units (view/prefs
    dialogue pagebase page keybindingpage contextmenulist
    )

opencs_units (model/prefs
    state setting intsetting doublesetting boolsetting enumsetting coloursetting shortcut
    shortcuteventhandler shortcutmanager shortcutsetting modifiersetting stringsetting subcategory
    )

opencs_units (model/prefs
    category
    )

opencs_units (model/filter
    unarynode narynode leafnode booleannode parser andnode ornode notnode textnode valuenode
    )

opencs_units (view/filter
    filterbox recordfilterbox editwidget
    )

set (OPENCS_US
    )

set (OPENCS_RES ${CMAKE_SOURCE_DIR}/files/opencs/resources.qrc
                ${CMAKE_SOURCE_DIR}/files/launcher/launcher.qrc
    )

set (OPENCS_UI
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/filedialog.ui
    )

source_group (openmw-cs FILES main.cpp ${OPENCS_SRC} ${OPENCS_HDR})

if(WIN32)
    set(OPENCS_RES ${OPENCS_RES} ${CMAKE_SOURCE_DIR}/files/windows/QWindowsVistaDark/dark.qrc)
    set(QT_USE_QTMAIN TRUE)
    set(OPENCS_RC_FILE ${CMAKE_SOURCE_DIR}/files/windows/opencs.rc)
else(WIN32)
    set(OPENCS_RC_FILE "")
endif(WIN32)

if (QT_VERSION_MAJOR VERSION_EQUAL 5)
    qt5_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
else ()
    qt6_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
endif()
qt_add_resources(OPENCS_RES_SRC ${OPENCS_RES})

# for compiled .ui files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    set (OPENCS_MAC_ICON "${CMAKE_SOURCE_DIR}/files/mac/openmw-cs.icns")
    set (OPENCS_CFG "${OpenMW_BINARY_DIR}/defaults-cs.bin")
    set (OPENCS_DEFAULT_FILTERS_FILE "${OpenMW_BINARY_DIR}/resources/defaultfilters")
    set (OPENCS_OPENMW_CFG "${OpenMW_BINARY_DIR}/openmw.cfg")
else()
    set (OPENCS_MAC_ICON "")
    set (OPENCS_CFG "")
    set (OPENCS_DEFAULT_FILTERS_FILE "")
    set (OPENCS_OPENMW_CFG "")
endif(APPLE)

add_library(openmw-cs-lib STATIC
    ${OPENCS_SRC}
    ${OPENCS_UI_HDR}
    ${OPENCS_MOC_SRC}
    ${OPENCS_RES_SRC}
)

if(BUILD_OPENCS)
    openmw_add_executable(openmw-cs
        MACOSX_BUNDLE
        ${OPENCS_MAC_ICON}
        ${OPENCS_CFG}
        ${OPENCS_DEFAULT_FILTERS_FILE}
        ${OPENCS_OPENMW_CFG}
        ${OPENCS_RC_FILE}
        main.cpp
    )

    target_link_libraries(openmw-cs openmw-cs-lib)

    if (BUILD_WITH_CODE_COVERAGE)
        target_compile_options(openmw-cs PRIVATE --coverage)
        target_link_libraries(openmw-cs gcov)
    endif()
endif()

if(APPLE AND BUILD_OPENCS)
    set(OPENCS_BUNDLE_NAME "OpenMW-CS")
    set(OPENCS_BUNDLE_RESOURCES_DIR "${OpenMW_BINARY_DIR}/${OPENCS_BUNDLE_NAME}.app/Contents/Resources")

    set(OPENMW_RESOURCES_ROOT ${OPENCS_BUNDLE_RESOURCES_DIR})

    add_subdirectory(../../files/ ${CMAKE_CURRENT_BINARY_DIR}/files)

    set_target_properties(openmw-cs PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}"
        OUTPUT_NAME ${OPENCS_BUNDLE_NAME}
        MACOSX_BUNDLE_ICON_FILE "openmw-cs.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "OpenMW-CS"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.openmw.opencs"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${OPENMW_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${OPENMW_VERSION}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/files/mac/openmw-cs-Info.plist.in"
        )

    set_source_files_properties(${OPENCS_MAC_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${OPENCS_CFG} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${OPENCS_DEFAULT_FILTERS_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources/resources)
    set_source_files_properties(${OPENCS_OPENMW_CFG} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)

    add_custom_command(TARGET openmw-cs
        POST_BUILD
        COMMAND cp "${OpenMW_BINARY_DIR}/resources/version" "${OPENCS_BUNDLE_RESOURCES_DIR}/resources")
endif()

target_link_libraries(openmw-cs-lib
    # CMake's built-in OSG finder does not use pkgconfig, so we have to
    # manually ensure the order is correct for inter-library dependencies.
    # This only makes a difference with `-DOPENMW_USE_SYSTEM_OSG=ON -DOSG_STATIC=ON`.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21701
    ${OSGVIEWER_LIBRARIES}
    ${OSGFX_LIBRARIES}
    ${OSGGA_LIBRARIES}
    ${OSGUTIL_LIBRARIES}
    ${OSGTEXT_LIBRARIES}
    ${OSG_LIBRARIES}
    ${EXTERN_OSGQT_LIBRARY}
    Boost::program_options
    components_qt
)

if (QT_VERSION_MAJOR VERSION_EQUAL 6)
    target_link_libraries(openmw-cs-lib Qt::Widgets Qt::Core Qt::Network Qt::OpenGL Qt::OpenGLWidgets Qt::Svg)
else()
    target_link_libraries(openmw-cs-lib Qt::Widgets Qt::Core Qt::Network Qt::OpenGL Qt::Svg)
endif()

if (WIN32)
    target_link_libraries(openmw-cs-lib Boost::locale)
    target_sources(openmw-cs PRIVATE ${CMAKE_SOURCE_DIR}/files/windows/openmw-cs.exe.manifest)
endif()

if (WIN32 AND BUILD_OPENCS)
    INSTALL(TARGETS openmw-cs RUNTIME DESTINATION ".")

    get_generator_is_multi_config(multi_config)
    if (multi_config)
        SET(INSTALL_SOURCE "${OpenMW_BINARY_DIR}/$<CONFIG>")
    else ()
        SET(INSTALL_SOURCE "${OpenMW_BINARY_DIR}")
    endif ()

    INSTALL(FILES "${INSTALL_SOURCE}/defaults-cs.bin" DESTINATION ".")
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
endif (MSVC)

if(APPLE AND BUILD_OPENCS)
    INSTALL(TARGETS openmw-cs BUNDLE DESTINATION "." COMPONENT Bundle)
endif()

if(USE_QT)
    set_property(TARGET openmw-cs-lib PROPERTY AUTOMOC ON)
endif(USE_QT)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-cs-lib PRIVATE --coverage)
    target_link_libraries(openmw-cs-lib gcov)
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw-cs-lib PRIVATE
        <boost/program_options/options_description.hpp>

        <algorithm>
        <fstream>
        <functional>
        <ostream>
        <string>
        <string_view>
        <vector>
    )
endif()
