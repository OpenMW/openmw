
set (OPENCS_SRC
    main.cpp editor.cpp

    model/doc/documentmanager.cpp model/doc/document.cpp

    model/world/universalid.cpp model/world/idcollection.cpp model/world/data.cpp model/world/idtable.cpp
    model/world/commands.cpp model/world/idtableproxymodel.cpp model/world/record.cpp
    model/world/columnbase.cpp

    model/tools/tools.cpp model/tools/operation.cpp model/tools/stage.cpp model/tools/verifier.cpp
    model/tools/mandatoryid.cpp model/tools/reportmodel.cpp

    view/doc/viewmanager.cpp view/doc/view.cpp view/doc/operations.cpp view/doc/operation.cpp view/doc/subviewfactory.cpp
    view/doc/subview.cpp

    view/world/table.cpp view/world/tablesubview.cpp view/world/subviews.cpp view/world/util.cpp
    view/world/dialoguesubview.cpp

    view/tools/reportsubview.cpp view/tools/subviews.cpp
    )

set (OPENCS_HDR
    editor.hpp

    model/doc/documentmanager.hpp model/doc/document.hpp model/doc/state.hpp

    model/world/universalid.hpp model/world/record.hpp model/world/idcollection.hpp model/world/data.hpp
    model/world/idtable.hpp model/world/columns.hpp model/world/idtableproxymodel.hpp
    model/world/commands.hpp model/world/columnbase.hpp

    model/tools/tools.hpp model/tools/operation.hpp model/tools/stage.hpp model/tools/verifier.hpp
    model/tools/mandatoryid.hpp model/tools/reportmodel.hpp

    view/doc/viewmanager.hpp view/doc/view.hpp view/doc/operations.hpp view/doc/operation.hpp view/doc/subviewfactory.hpp
    view/doc/subview.hpp view/doc/subviewfactoryimp.hpp

    view/world/table.hpp view/world/tablesubview.hpp view/world/subviews.hpp view/world/util.hpp
    view/world/dialoguesubview.hpp

    view/tools/reportsubview.hpp view/tools/subviews.hpp
    )

set (OPENCS_US
    )

set (OPENCS_RES
    )

source_group (opencs FILES ${OPENCS_SRC} ${OPENCS_HDR})

if(WIN32)
    set(QT_USE_QTMAIN TRUE)
endif(WIN32)

find_package(Qt4 COMPONENTS QtCore QtGui QtXml QtXmlPatterns REQUIRED)
include(${QT_USE_FILE})

qt4_wrap_ui(OPENCS_UI_HDR ${OPENCS_UI})
qt4_wrap_cpp(OPENCS_MOC_SRC ${OPENCS_HDR})
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