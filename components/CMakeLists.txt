project (Components)
set (CMAKE_BUILD_TYPE DEBUG)

# Version file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version/version.hpp.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/version/version.hpp")

# source files

add_component_dir (settings
    settings
    )

add_component_dir (nifoverrides
    nifoverrides
    )

add_component_dir (bsa
    bsa_archive bsa_file resources
    )

add_component_dir (nif
    controlled effect niftypes record controller extra node record_ptr data niffile property
    )

add_component_dir (nifogre
    ogrenifloader skeleton material mesh particles controller
    )

add_component_dir (nifbullet
    bulletnifloader
    )

add_component_dir (to_utf8
    to_utf8
    )

add_component_dir (esm
    attr defs esmcommon esmreader esmwriter loadacti loadalch loadappa loadarmo loadbody loadbook loadbsgn loadcell
    loadclas loadclot loadcont loadcrea loadcrec loaddial loaddoor loadench loadfact loadglob loadgmst
    loadinfo loadingr loadland loadlevlist loadligh loadlock loadprob loadrepa loadltex loadmgef loadmisc loadnpcc
    loadnpc loadpgrd loadrace loadregn loadscpt loadskil loadsndg loadsoun loadspel loadsscr loadstat
    loadweap records aipackage effectlist spelllist variant variantimp loadtes3 cellref filter
    savedgame journalentry queststate locals globalscript player objectstate cellid cellstate globalmap lightstate inventorystate containerstate npcstate creaturestate dialoguestate statstate
    npcstats creaturestats weatherstate quickkeys fogstate spellstate activespells creaturelevliststate doorstate projectilestate
    aisequence
    )

add_component_dir (misc
    utf8stream stringops
    )

add_component_dir (files
    linuxpath windowspath macospath fixedpath multidircollection collections configurationmanager
    constrainedfiledatastream lowlevelfile
    )

add_component_dir (compiler
    context controlparser errorhandler exception exprparser extensions fileparser generator
    lineparser literals locals output parser scanner scriptparser skipparser streamerrorhandler
    stringparser tokenloc nullerrorhandler opcodes extensions0 declarationparser
    quickfileparser discardparser
    )

add_component_dir (interpreter
    context controlopcodes genericopcodes installopcodes interpreter localopcodes mathopcodes
    miscopcodes opcodes runtime scriptopcodes spatialopcodes types defines
    )

add_component_dir (translation
    translation
    )

add_definitions(-DTERRAIN_USE_SHADER=1)
add_component_dir (terrain
    quadtreenode chunk world defaultworld storage material buffercache defs
    )

add_component_dir (loadinglistener
    loadinglistener
    )

add_component_dir (ogreinit
    ogreinit ogreplugin
    )

add_component_dir (version
    version
    )

set (ESM_UI ${CMAKE_SOURCE_DIR}/files/ui/contentselector.ui
    )

find_package(Qt4 COMPONENTS QtCore QtGui)

if(QT_QTGUI_LIBRARY AND QT_QTCORE_LIBRARY)
   add_component_qt_dir (contentselector
        model/modelitem model/esmfile
        model/naturalsort model/contentmodel
        view/combobox view/contentselector
       )

   include(${QT_USE_FILE})
    QT4_WRAP_UI(ESM_UI_HDR ${ESM_UI})
   QT4_WRAP_CPP(MOC_SRCS ${COMPONENT_MOC_FILES})
endif(QT_QTGUI_LIBRARY AND QT_QTCORE_LIBRARY)

include_directories(${BULLET_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

add_library(components STATIC ${COMPONENT_FILES} ${MOC_SRCS} ${ESM_UI_HDR})

target_link_libraries(components ${Boost_LIBRARIES} ${OGRE_LIBRARIES})

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
target_link_libraries(components ${CMAKE_THREAD_LIBS_INIT})
endif()


# Make the variable accessible for other subdirectories
set(COMPONENT_FILES ${COMPONENT_FILES} PARENT_SCOPE)
