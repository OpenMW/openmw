
set (OPENCS_SRC
    main.cpp editor.cpp

    model/doc/documentmanager.cpp model/doc/document.cpp

    view/doc/viewmanager.cpp view/doc/view.cpp view/doc/operations.cpp view/doc/operation.cpp
    )

set (OPENCS_HDR
    editor.hpp

    model/doc/documentmanager.hpp model/doc/document.hpp

    view/doc/viewmanager.hpp view/doc/view.hpp view/doc/operations.hpp view/doc/operation.hpp
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