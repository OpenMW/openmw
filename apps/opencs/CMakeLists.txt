set (OPENCS_SRC main.cpp)

opencs_units (. editor)

set (CMAKE_BUILD_TYPE DEBUG)

opencs_units (model/doc
    document
    )

opencs_units_noqt (model/doc
    documentmanager
    )

opencs_hdrs_noqt (model/doc
    state
    )


opencs_units (model/world
    idtable idtableproxymodel
    )


opencs_units_noqt (model/world
    universalid data record idcollection commands columnbase scriptcontext cell refidcollection
    refidadapter refiddata refidadapterimp
    )

opencs_hdrs_noqt (model/world
    columns
    )


opencs_units (model/tools
    tools operation reportmodel
    )

opencs_units_noqt (model/tools
    stage verifier mandatoryid skillcheck classcheck factioncheck racecheck soundcheck regioncheck
    birthsigncheck spellcheck
    )


opencs_units (view/doc
    viewmanager view operations operation subview startup filedialog
    )


opencs_units_noqt (view/doc
    subviewfactory
    )

opencs_hdrs_noqt (view/doc
    subviewfactoryimp
    )


opencs_units (view/world
    table tablesubview scriptsubview util
    )

opencs_units_noqt (view/world
    dialoguesubview subviews enumdelegate vartypedelegate scripthighlighter recordstatusdelegate
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
    editorpage
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

set (OPENCS_US
    )

set (OPENCS_RES ../../files/opencs/resources.qrc
                ../../files/launcher/launcher.qrc
    )

set (OPENCS_UI ../../files/ui/datafilespage.ui
    )

source_group (opencs FILES ${OPENCS_SRC} ${OPENCS_HDR})

if(WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

find_package(Qt4 COMPONENTS QtCore QtGui QtXml QtXmlPatterns REQUIRED)
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
