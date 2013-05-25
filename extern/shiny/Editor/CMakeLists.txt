set(SHINY_EDITOR_LIBRARY "shiny.Editor")

find_package(Qt4)

if (QT_FOUND)

    add_definitions(-DSHINY_BUILD_MATERIAL_EDITOR=1)
    set (SHINY_BUILD_EDITOR_FLAG -DSHINY_BUILD_MATERIAL_EDITOR=1 PARENT_SCOPE)

    set(QT_USE_QTGUI 1)

    # Headers that must be preprocessed
    set(SHINY_EDITOR_HEADER_MOC
        MainWindow.hpp
        NewMaterialDialog.hpp
        AddPropertyDialog.hpp
        PropertySortModel.hpp
    )

    set(SHINY_EDITOR_UI
        mainwindow.ui
        newmaterialdialog.ui
        addpropertydialog.ui
    )

    QT4_WRAP_CPP(MOC_SRCS ${SHINY_EDITOR_HEADER_MOC})
    QT4_WRAP_UI(UI_HDRS ${SHINY_EDITOR_UI})

    set(SOURCE_FILES
        NewMaterialDialog.cpp
        AddPropertyDialog.cpp
        ColoredTabWidget.hpp
        MainWindow.cpp
        Editor.cpp
        Actions.cpp
        Query.cpp
        PropertySortModel.cpp
        ${SHINY_EDITOR_UI} # Just to have them in the IDE's file explorer
    )

    include(${QT_USE_FILE})

    set (CMAKE_INCLUDE_CURRENT_DIR "true")

    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    add_library(${SHINY_EDITOR_LIBRARY} STATIC ${SOURCE_FILES} ${MOC_SRCS} ${UI_HDRS})

    set(SHINY_LIBRARIES ${SHINY_LIBRARIES}
        ${SHINY_EDITOR_LIBRARY}
        ${QT_LIBRARIES}
    )
    set(SHINY_LIBRARIES ${SHINY_LIBRARIES} PARENT_SCOPE)

else (QT_FOUND)

    add_definitions(-DSHINY_BUILD_MATERIAL_EDITOR=0)
    set (SHINY_BUILD_EDITOR_FLAG -DSHINY_BUILD_MATERIAL_EDITOR=0 PARENT_SCOPE)
    message(STATUS "QT4 was not found. You will not be able to use the material editor.")

endif(QT_FOUND)
