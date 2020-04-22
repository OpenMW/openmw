set (OPENCS_SRC main.cpp
    ${CMAKE_SOURCE_DIR}/files/windows/opencs.rc
    )

opencs_units (. editor)

opencs_units (model/doc
    document operation saving documentmanager loader runner operationholder
    )

opencs_units_noqt (model/doc
    stage savingstate savingstages blacklist messages
    )

opencs_hdrs_noqt (model/doc
    state
    )


opencs_units (model/world
    idtable idtableproxymodel regionmap data commanddispatcher idtablebase resourcetable nestedtableproxymodel idtree infotableproxymodel landtexturetableproxymodel
    actoradapter
    )


opencs_units_noqt (model/world
    universalid record commands columnbase columnimp scriptcontext cell refidcollection
    refidadapter refiddata refidadapterimp ref collectionbase refcollection columns infocollection tablemimedata cellcoordinates cellselection resources resourcesmanager scope
    pathgrid landtexture land nestedtablewrapper nestedcollection nestedcoladapterimp nestedinfocollection
    idcompletionmanager metadata defaultgmsts infoselectwrapper commandmacro
    )

opencs_hdrs_noqt (model/world
    columnimp idcollection collection info subcellcollection
    )


opencs_units (model/tools
    tools reportmodel mergeoperation
    )

opencs_units_noqt (model/tools
    mandatoryid skillcheck classcheck factioncheck racecheck soundcheck regioncheck
    birthsigncheck spellcheck referencecheck referenceablecheck scriptcheck bodypartcheck
    startscriptcheck search searchoperation searchstage pathgridcheck soundgencheck magiceffectcheck
    mergestages gmstcheck topicinfocheck journalcheck enchantmentcheck
    )

opencs_hdrs_noqt (model/tools
    mergestate
    )


opencs_units (view/doc
    viewmanager view operations operation subview startup filedialog newgame
    filewidget adjusterwidget loader globaldebugprofilemenu runlogsubview sizehint
    )


opencs_units_noqt (view/doc
    subviewfactory
    )

opencs_hdrs_noqt (view/doc
    subviewfactoryimp
    )


opencs_units (view/world
    table tablesubview scriptsubview util regionmapsubview tablebottombox creator genericcreator globalcreator
    cellcreator pathgridcreator referenceablecreator startscriptcreator referencecreator scenesubview
    infocreator scriptedit dialoguesubview previewsubview regionmap dragrecordtable nestedtable
    dialoguespinbox recordbuttonbar tableeditidaction scripterrortable extendedcommandconfigurator
    bodypartcreator landtexturecreator landcreator
    )

opencs_units_noqt (view/world
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
    cellwater terraintexturemode actor terrainselection terrainshapemode brushdraw
    )

opencs_units_noqt (view/render
    lighting lightingday lightingnight lightingbright object cell terrainstorage tagbase
    cellarrow cellmarker cellborder pathgrid
    )


opencs_units (view/tools
    reportsubview reporttable searchsubview searchbox merge
    )

opencs_units_noqt (view/tools
    subviews
    )

opencs_units (view/prefs
    dialogue pagebase page keybindingpage contextmenulist
    )

opencs_units (model/prefs
    state setting intsetting doublesetting boolsetting enumsetting coloursetting shortcut
    shortcuteventhandler shortcutmanager shortcutsetting modifiersetting
    )

opencs_units_noqt (model/prefs
    category
    )

opencs_units_noqt (model/filter
    node unarynode narynode leafnode booleannode parser andnode ornode notnode textnode valuenode
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
    ${CMAKE_SOURCE_DIR}/files/ui/contentselector.ui
    ${CMAKE_SOURCE_DIR}/files/ui/filedialog.ui
    )

source_group (openmw-cs FILES ${OPENCS_SRC} ${OPENCS_HDR})

if(WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

if (DESIRED_QT_VERSION MATCHES 4)
    include(${QT_USE_FILE})
    qt4_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
    qt4_wrap_cpp(OPENCS_MOC_SRC ${OPENCS_HDR_QT})
    qt4_add_resources(OPENCS_RES_SRC ${OPENCS_RES})
else()
    qt5_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
    qt5_wrap_cpp(OPENCS_MOC_SRC ${OPENCS_HDR_QT})
    qt5_add_resources(OPENCS_RES_SRC ${OPENCS_RES})
endif()

# for compiled .ui files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    set (OPENCS_MAC_ICON "${CMAKE_SOURCE_DIR}/files/mac/openmw-cs.icns")
    set (OPENCS_CFG "${OpenMW_BINARY_DIR}/openmw-cs.cfg")
    set (OPENCS_DEFAULT_FILTERS_FILE "${OpenMW_BINARY_DIR}/resources/defaultfilters")
    set (OPENCS_OPENMW_CFG "${OpenMW_BINARY_DIR}/openmw.cfg")
else()
    set (OPENCS_MAC_ICON "")
    set (OPENCS_CFG "")
    set (OPENCS_DEFAULT_FILTERS_FILE "")
    set (OPENCS_OPENMW_CFG "")
endif(APPLE)

openmw_add_executable(openmw-cs
    MACOSX_BUNDLE
    ${OPENCS_SRC}
    ${OPENCS_UI_HDR}
    ${OPENCS_MOC_SRC}
    ${OPENCS_RES_SRC}
    ${OPENCS_MAC_ICON}
    ${OPENCS_CFG}
    ${OPENCS_DEFAULT_FILTERS_FILE}
    ${OPENCS_OPENMW_CFG}
)

if(APPLE)
    set(OPENCS_BUNDLE_NAME "OpenMW-CS")
    set(OPENCS_BUNDLE_RESOURCES_DIR "${OpenMW_BINARY_DIR}/${OPENCS_BUNDLE_NAME}.app/Contents/Resources")

    set(OPENMW_MYGUI_FILES_ROOT ${OPENCS_BUNDLE_RESOURCES_DIR})
    set(OPENMW_SHADERS_ROOT ${OPENCS_BUNDLE_RESOURCES_DIR})

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
endif(APPLE)

target_link_libraries(openmw-cs
    ${OSG_LIBRARIES}
    ${OPENTHREADS_LIBRARIES}
    ${OSGTEXT_LIBRARIES}
    ${OSGUTIL_LIBRARIES}
    ${OSGVIEWER_LIBRARIES}
    ${OSGGA_LIBRARIES}
    ${OSGFX_LIBRARIES}
    ${EXTERN_OSGQT_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    components
)

if (DESIRED_QT_VERSION MATCHES 4)
    target_link_libraries(openmw-cs
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTOPENGL_LIBRARY})

    if (WIN32)
        target_link_libraries(openmw-cs ${QT_QTMAIN_LIBRARY})
    endif()
else()
    target_link_libraries(openmw-cs Qt5::Widgets Qt5::Core Qt5::Network Qt5::OpenGL)
endif()

if (WIN32)
    target_link_libraries(openmw-cs ${Boost_LOCALE_LIBRARY})
    INSTALL(TARGETS openmw-cs RUNTIME DESTINATION ".")
    INSTALL(FILES "${OpenMW_BINARY_DIR}/Debug/openmw-cs.cfg" DESTINATION "." CONFIGURATIONS Debug)
    INSTALL(FILES "${OpenMW_BINARY_DIR}/Release/openmw-cs.cfg" DESTINATION "." CONFIGURATIONS Release;RelWithDebInfo;MinSizeRel)
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
endif (MSVC)


if(APPLE)
    INSTALL(TARGETS openmw-cs BUNDLE DESTINATION "." COMPONENT Bundle)
endif()
