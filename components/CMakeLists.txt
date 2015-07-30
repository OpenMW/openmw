project (Components)

# Version file
set (VERSION_HPP_IN ${CMAKE_CURRENT_SOURCE_DIR}/version/version.hpp.cmake)
set (VERSION_HPP ${CMAKE_CURRENT_SOURCE_DIR}/version/version.hpp)
if (GIT_CHECKOUT)
    add_custom_target (git-version
        COMMAND ${CMAKE_COMMAND}
	    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
	    -DVERSION_HPP_IN=${VERSION_HPP_IN}
	    -DVERSION_HPP=${VERSION_HPP}
	    -DOPENMW_VERSION_MAJOR=${OPENMW_VERSION_MAJOR}
	    -DOPENMW_VERSION_MINOR=${OPENMW_VERSION_MINOR}
	    -DOPENMW_VERSION_RELEASE=${OPENMW_VERSION_RELEASE}
	    -DOPENMW_VERSION=${OPENMW_VERSION}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/GitVersion.cmake
	    VERBATIM)
else (GIT_CHECKOUT)
    configure_file(${VERSION_HPP_IN} ${VERSION_HPP})
endif (GIT_CHECKOUT)

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
    controlled effect niftypes record controller extra node record_ptr data niffile property nifkey data node base nifstream
    )

add_component_dir (nifcache
    nifcache
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
    loadclas loadclot loadcont loadcrea loaddial loaddoor loadench loadfact loadglob loadgmst
    loadinfo loadingr loadland loadlevlist loadligh loadlock loadprob loadrepa loadltex loadmgef loadmisc
    loadnpc loadpgrd loadrace loadregn loadscpt loadskil loadsndg loadsoun loadspel loadsscr loadstat
    loadweap records aipackage effectlist spelllist variant variantimp loadtes3 cellref filter
    savedgame journalentry queststate locals globalscript player objectstate cellid cellstate globalmap inventorystate containerstate npcstate creaturestate dialoguestate statstate
    npcstats creaturestats weatherstate quickkeys fogstate spellstate activespells creaturelevliststate doorstate projectilestate debugprofile
    aisequence magiceffects util custommarkerstate stolenitems transport
    )

add_component_dir (esmterrain
    storage
    )

add_component_dir (misc
    utf8stream stringops resourcehelpers
    )

IF(NOT WIN32 AND NOT APPLE)
    add_definitions(-DGLOBAL_DATA_PATH="${GLOBAL_DATA_PATH}")
    add_definitions(-DGLOBAL_CONFIG_PATH="${GLOBAL_CONFIG_PATH}")
ENDIF()
add_component_dir (files
    linuxpath androidpath windowspath macospath fixedpath multidircollection collections configurationmanager
    constrainedfiledatastream lowlevelfile
    )

add_component_dir (compiler
    context controlparser errorhandler exception exprparser extensions fileparser generator
    lineparser literals locals output parser scanner scriptparser skipparser streamerrorhandler
    stringparser tokenloc nullerrorhandler opcodes extensions0 declarationparser
    quickfileparser discardparser junkparser
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
    quadtreenode chunk world defaultworld terraingrid storage material buffercache defs
    )

add_component_dir (loadinglistener
    loadinglistener
    )

add_component_dir (ogreinit
    ogreinit ogreplugin
    )

add_component_dir (widgets
    box imagebutton tags list numericeditbox sharedstatebutton windowcaption widgets
    )

add_component_dir (fontloader
    fontloader
    )

add_component_dir (autocalc
    autocalc autocalcspell
    )

add_component_dir (version
    version
    )

set (ESM_UI ${CMAKE_SOURCE_DIR}/files/ui/contentselector.ui
    )

add_component_qt_dir (contentselector
    model/modelitem model/esmfile
    model/naturalsort model/contentmodel
    model/loadordererror
    view/combobox view/contentselector
    )
add_component_qt_dir (config
    gamesettings
    launchersettings
    settingsbase
    )

add_component_qt_dir (process
    processinvoker
)

if (DESIRED_QT_VERSION MATCHES 4)
    include(${QT_USE_FILE})
    QT4_WRAP_UI(ESM_UI_HDR ${ESM_UI})
    QT4_WRAP_CPP(MOC_SRCS ${COMPONENT_MOC_FILES})
else()
    QT5_WRAP_UI(ESM_UI_HDR ${ESM_UI})
    QT5_WRAP_CPP(MOC_SRCS ${COMPONENT_MOC_FILES})
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" AND NOT APPLE)
        add_definitions(-fPIC)
    endif()
endif ()

include_directories(${BULLET_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

add_library(components STATIC ${COMPONENT_FILES} ${MOC_SRCS} ${ESM_UI_HDR})

target_link_libraries(components 
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_WAVE_LIBRARY}
    ${OGRE_LIBRARIES}
    ${OENGINE_LIBRARY}    
    ${BULLET_LIBRARIES}
)

if (WIN32)
    target_link_libraries(components
    ${Boost_LOCALE_LIBRARY})
endif()

if (DESIRED_QT_VERSION MATCHES 4)
    target_link_libraries(components
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY})
else()
    qt5_use_modules(components Widgets Core)
endif()

if (GIT_CHECKOUT)
    add_dependencies (components git-version)
endif (GIT_CHECKOUT)

if (WIN32)
    target_link_libraries(components shlwapi)
    if(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOGDI")
    endif(MINGW)
endif()

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
target_link_libraries(components ${CMAKE_THREAD_LIBS_INIT})
endif()


# Make the variable accessible for other subdirectories
set(COMPONENT_FILES ${COMPONENT_FILES} PARENT_SCOPE)
