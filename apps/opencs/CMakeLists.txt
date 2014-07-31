set (OPENCS_SRC main.cpp)

opencs_units (. editor)

set (CMAKE_BUILD_TYPE DEBUG)

opencs_units (model/doc
    document operation saving documentmanager loader
    )

opencs_units_noqt (model/doc
    stage savingstate savingstages blacklist
    )

opencs_hdrs_noqt (model/doc
    state
    )


opencs_units (model/world
    idtable idtableproxymodel regionmap data commanddispatcher idtablebase resourcetable
    )


opencs_units_noqt (model/world
    universalid record commands columnbase scriptcontext cell refidcollection
    refidadapter refiddata refidadapterimp ref collectionbase refcollection columns infocollection tablemimedata cellcoordinates cellselection resources resourcesmanager
    )

opencs_hdrs_noqt (model/world
    columnimp idcollection collection info
    )


opencs_units (model/tools
    tools reportmodel
    )

opencs_units_noqt (model/tools
    mandatoryid skillcheck classcheck factioncheck racecheck soundcheck regioncheck
    birthsigncheck spellcheck referenceablecheck scriptcheck
    )


opencs_units (view/doc
    viewmanager view operations operation subview startup filedialog newgame
    filewidget adjusterwidget loader
    )


opencs_units_noqt (view/doc
    subviewfactory
    )

opencs_hdrs_noqt (view/doc
    subviewfactoryimp
    )


opencs_units (view/world
    table tablesubview scriptsubview util regionmapsubview tablebottombox creator genericcreator
    cellcreator referenceablecreator referencecreator scenesubview
    infocreator scriptedit dialoguesubview previewsubview regionmap dragrecordtable
    )

opencs_units_noqt (view/world
    subviews enumdelegate vartypedelegate recordstatusdelegate idtypedelegate datadisplaydelegate
    scripthighlighter idvalidator dialoguecreator
    )

opencs_units (view/widget
    scenetoolbar scenetool scenetoolmode pushbutton scenetooltoggle
    )

opencs_units (view/render
    scenewidget worldspacewidget pagedworldspacewidget unpagedworldspacewidget
    previewwidget
    )

opencs_units_noqt (view/render
    navigation navigation1st navigationfree navigationorbit lighting lightingday lightingnight
    lightingbright object cell
    )

opencs_hdrs_noqt (view/render
    elements
    )


opencs_units (view/tools
    reportsubview
    )

opencs_units_noqt (view/tools
    subviews
    )

opencs_units (view/settings
    settingwindow
    dialog
    page
    view
    booleanview
    textview
    listview
    rangeview
    resizeablestackedwidget
    spinbox
    )

opencs_units_noqt (view/settings
    frame
    )

opencs_units (model/settings
    usersettings
    setting
    connector
    )

opencs_hdrs_noqt (model/settings
    support
    )

opencs_units_noqt (model/filter
    node unarynode narynode leafnode booleannode parser andnode ornode notnode textnode valuenode
    )

opencs_hdrs_noqt (model/filter
    filter
    )

opencs_units (view/filter
    filtercreator filterbox recordfilterbox editwidget
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

source_group (opencs FILES ${OPENCS_SRC} ${OPENCS_HDR})

if(WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

set(BOOST_COMPONENTS system filesystem program_options thread wave)
if(WIN32)
    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} locale)
endif(WIN32)

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork REQUIRED)
include(${QT_USE_FILE})

qt4_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
qt4_wrap_cpp(OPENCS_MOC_SRC ${OPENCS_HDR_QT})
qt4_add_resources(OPENCS_RES_SRC ${OPENCS_RES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
    set (OPENCS_MAC_ICON ${CMAKE_SOURCE_DIR}/files/mac/opencs.icns)
else()
    set (OPENCS_MAC_ICON "")
endif(APPLE)

add_executable(opencs
    MACOSX_BUNDLE
    ${OPENCS_SRC}
    ${OPENCS_UI_HDR}
    ${OPENCS_MOC_SRC}
    ${OPENCS_RES_SRC}
    ${OPENCS_MAC_ICON}
)

if(APPLE)
    set_target_properties(opencs PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OpenMW_BINARY_DIR}"
        OUTPUT_NAME "OpenCS"
        MACOSX_BUNDLE_ICON_FILE "opencs.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "OpenCS"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.openmw.opencs"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${OPENMW_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${OPENMW_VERSION}
        )

    set_source_files_properties(${OPENCS_MAC_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
endif(APPLE)

target_link_libraries(opencs
    ${OGRE_LIBRARIES}
    ${OGRE_STATIC_PLUGINS}
    ${SHINY_LIBRARIES}
    ${Boost_LIBRARIES}
    ${QT_LIBRARIES}
    components
)

if(APPLE)
    INSTALL(TARGETS opencs BUNDLE DESTINATION OpenMW COMPONENT BUNDLE)
endif()
