set (OPENCS_SRC main.cpp)

opencs_units (. editor)

set (CMAKE_BUILD_TYPE DEBUG)

opencs_units (model/doc
    document operation saving
    )

opencs_units_noqt (model/doc
    documentmanager stage savingstate savingstages
    )

opencs_hdrs_noqt (model/doc
    state
    )


opencs_units (model/world
    idtable idtableproxymodel regionmap data
    )


opencs_units_noqt (model/world
    universalid record commands columnbase scriptcontext cell refidcollection
    refidadapter refiddata refidadapterimp ref collectionbase refcollection columns
    )

opencs_hdrs_noqt (model/world
    columnimp idcollection collection
    )


opencs_units (model/tools
    tools reportmodel
    )

opencs_units_noqt (model/tools
    mandatoryid skillcheck classcheck factioncheck racecheck soundcheck regioncheck
    birthsigncheck spellcheck
    )


opencs_units (view/doc
    viewmanager view operations operation subview startup filedialog newgame
    filewidget adjusterwidget
    )


opencs_units_noqt (view/doc
    subviewfactory
    )

opencs_hdrs_noqt (view/doc
    subviewfactoryimp
    )


opencs_units (view/world
    table tablesubview scriptsubview util regionmapsubview tablebottombox creator genericcreator
    cellcreator referenceablecreator referencecreator scenesubview scenetoolbar scenetool
    scenetoolmode
    )

opencs_units_noqt (view/world
    dialoguesubview subviews
    enumdelegate vartypedelegate recordstatusdelegate idtypedelegate datadisplaydelegate
    scripthighlighter idvalidator dialoguecreator
    )


opencs_units (view/tools
    reportsubview
    )

opencs_units_noqt (view/tools
    subviews
    )

opencs_units (view/settings
    abstractblock
    proxyblock
    abstractwidget
    usersettingsdialog
    datadisplayformatpage
    windowpage
    )

opencs_units_noqt (view/settings
    abstractpage
    blankpage
    groupblock
    customblock
    groupbox
    itemblock
    settingwidget
    toggleblock
    support
    )

opencs_units (model/settings
    usersettings
    settingcontainer
    )

opencs_units_noqt (model/settings
    support
    settingsitem
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

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork QtXml QtXmlPatterns REQUIRED)
include(${QT_USE_FILE})

qt4_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
qt4_wrap_cpp(OPENCS_MOC_SRC ${OPENCS_HDR_QT})
qt4_add_resources(OPENCS_RES_SRC ${OPENCS_RES})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(opencs
    ${OPENCS_SRC}
    ${OPENCS_UI_HDR}
    ${OPENCS_MOC_SRC}
    ${OPENCS_RES_SRC}
)

target_link_libraries(opencs
    ${Boost_LIBRARIES}
    ${QT_LIBRARIES}
    components
)

if(DPKG_PROGRAM)
    INSTALL(TARGETS opencs RUNTIME DESTINATION games COMPONENT opencs)
endif()

