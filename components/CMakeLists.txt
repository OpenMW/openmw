project (Components)

if(APPLE)
    set(BUNDLE_RESOURCES_DIR "${APP_BUNDLE_DIR}/Contents/Resources")
    set(OPENMW_RESOURCES_ROOT ${BUNDLE_RESOURCES_DIR})
endif(APPLE)

# Version file
set (VERSION_RESOURCE_FILE_IN "${OpenMW_SOURCE_DIR}/files/version.in")
set (VERSION_RESOURCE_FILE_RELATIVE "resources/version")
set (VERSION_CPP_FILE "components/version/version.cpp")

if (GIT_CHECKOUT)
    get_generator_is_multi_config(multi_config)
    add_custom_target (get-version
       BYPRODUCTS "${OpenMW_BINARY_DIR}/${VERSION_CPP_FILE}"
       DEPENDS "${OpenMW_SOURCE_DIR}/${VERSION_CPP_FILE}.in"
       COMMAND ${CMAKE_COMMAND}
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DOpenMW_SOURCE_DIR=${OpenMW_SOURCE_DIR}
            -DOpenMW_BINARY_DIR=${OpenMW_BINARY_DIR}
            -DVERSION_RESOURCE_FILE_IN=${VERSION_RESOURCE_FILE_IN}
            -DVERSION_RESOURCE_FILE_RELATIVE=${VERSION_RESOURCE_FILE_RELATIVE}
            -DVERSION_CPP_FILE_IN=${OpenMW_SOURCE_DIR}/${VERSION_CPP_FILE}.in
            -DVERSION_CPP_FILE_OUT=${OpenMW_BINARY_DIR}/${VERSION_CPP_FILE}.out
            -DOPENMW_VERSION_MAJOR=${OPENMW_VERSION_MAJOR}
            -DOPENMW_VERSION_MINOR=${OPENMW_VERSION_MINOR}
            -DOPENMW_VERSION_RELEASE=${OPENMW_VERSION_RELEASE}
            -DOPENMW_LUA_API_REVISION=${OPENMW_LUA_API_REVISION}
            -DOPENMW_POSTPROCESSING_API_REVISION=${OPENMW_POSTPROCESSING_API_REVISION}
            -DOPENMW_VERSION=${OPENMW_VERSION}
            -DOPENMW_DOC_BASEURL=${OPENMW_DOC_BASEURL}
            -DMACROSFILE=${CMAKE_SOURCE_DIR}/cmake/OpenMWMacros.cmake
            "-DCMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}"
            -Dgenerator_is_multi_config_var=${multi_config}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/GitVersion.cmake
        COMMAND ${CMAKE_COMMAND}
            -E copy_if_different ${OpenMW_BINARY_DIR}/${VERSION_CPP_FILE}.out ${OpenMW_BINARY_DIR}/${VERSION_CPP_FILE}
        VERBATIM)
else (GIT_CHECKOUT)
    configure_resource_file(${VERSION_RESOURCE_FILE_IN} ${OpenMW_BINARY_DIR} ${VERSION_RESOURCE_FILE_RELATIVE})
    configure_file("${OpenMW_SOURCE_DIR}/${VERSION_CPP_FILE}.in" "${OpenMW_BINARY_DIR}/${VERSION_CPP_FILE}")
endif (GIT_CHECKOUT)

list (APPEND COMPONENT_FILES "${OpenMW_BINARY_DIR}/${VERSION_CPP_FILE}")

# OSG plugin checker
# Helpfully, OSG doesn't export this to its CMake config as it doesn't have one
list(TRANSFORM USED_OSG_PLUGINS REPLACE "^osgdb_" "" OUTPUT_VARIABLE USED_OSG_PLUGIN_NAMES)
list(TRANSFORM USED_OSG_PLUGIN_NAMES PREPEND "\"" OUTPUT_VARIABLE USED_OSG_PLUGIN_NAMES_FORMATTED)
list(TRANSFORM USED_OSG_PLUGIN_NAMES_FORMATTED APPEND "\"")
list(JOIN USED_OSG_PLUGIN_NAMES_FORMATTED ", " USED_OSG_PLUGIN_NAMES_FORMATTED)

set(OSG_PLUGIN_CHECKER_CPP_FILE "components/misc/osgpluginchecker.cpp")
configure_file("${OpenMW_SOURCE_DIR}/${OSG_PLUGIN_CHECKER_CPP_FILE}.in" "${OpenMW_BINARY_DIR}/${OSG_PLUGIN_CHECKER_CPP_FILE}")
list(APPEND COMPONENT_FILES "${OpenMW_BINARY_DIR}/${OSG_PLUGIN_CHECKER_CPP_FILE}")

# source files

add_component_dir (lua
    luastate scriptscontainer asyncpackage utilpackage serialization configuration l10n storage utf8
    shapes/box inputactions yamlloader scripttracker
    )

add_component_dir (l10n
    messagebundles manager
    )

add_component_dir (settings
    categories
    categories/camera
    categories/cells
    categories/fog
    categories/game
    categories/general
    categories/groundcover
    categories/gui
    categories/hud
    categories/input
    categories/lua
    categories/map
    categories/models
    categories/navigator
    categories/physics
    categories/postprocessing
    categories/saves
    categories/shaders
    categories/shadows
    categories/sound
    categories/stereo
    categories/stereoview
    categories/terrain
    categories/video
    categories/water
    categories/windows
    gyroscopeaxis
    hrtfmode
    navmeshrendermode
    parser
    sanitizer
    sanitizerimpl
    settings
    settingvalue
    shadermanager
    values
    windowmode
    )

add_component_dir (bgsm
    stream file
    )

add_component_dir (bsa
    bsafile compressedbsafile ba2gnrlfile ba2dx10file ba2file memorystream
    )

add_component_dir (bullethelpers
    collisionobject
    heightfield
    operators
    processtrianglecallback
    transformboundingbox
    )

add_component_dir (vfs
    manager archive bsaarchive filesystemarchive pathutil registerarchives
    )

add_component_dir (resource
    scenemanager keyframemanager imagemanager animblendrulesmanager bulletshapemanager bulletshape niffilemanager objectcache multiobjectcache resourcesystem
    resourcemanager stats animation foreachbulletobject errormarker cachestats bgsmfilemanager
    )

add_component_dir (shader
    shadermanager shadervisitor removedalphafunc
    )

add_component_dir (sceneutil
    clone attach visitor util statesetupdater controller skeleton riggeometry morphgeometry lightcontroller
    lightmanager lightutil positionattitudetransform workqueue pathgridutil waterutil writescene serialize optimizer
    detourdebugdraw navmesh agentpath animblendrules shadow mwshadowtechnique recastmesh shadowsbin osgacontroller rtt
    screencapture depth color riggeometryosgaextension extradata unrefqueue lightcommon lightingmethod clearcolor
    cullsafeboundsvisitor keyframe nodecallback textkeymap glextensions
    )

add_component_dir (nif
    base controller data effect extra niffile nifkey nifstream niftypes node parent particle physics property record recordptr texture
    )

add_component_dir (nifosg
    nifloader controller particle matrixtransform fog
    )

add_component_dir (nifbullet
    bulletnifloader
    )

add_component_dir (to_utf8
    tables_gen
    to_utf8
    )

add_component_dir(esm attr common defs esmcommon records util luascripts format refid esmbridge esmterrain
    formid
    fourcc
    stringrefid
    generatedrefid
    indexrefid
    serializerefid
    esm3exteriorcellrefid
    quaternion
    vector3
    exteriorcelllocation
    )

add_component_dir(fx pass technique lexer lexertypes parseconstants widgets stateupdater)

add_component_dir(std140 ubo)

add_component_dir (esm3
    esmreader esmwriter loadacti loadalch loadappa loadarmo loadbody loadbook loadbsgn loadcell
    loadclas loadclot loadcont loadcrea loaddial loaddoor loadench loadfact loadglob loadgmst
    loadinfo loadingr loadland loadlevlist loadligh loadlock loadprob loadrepa loadltex loadmgef loadmisc
    loadnpc loadpgrd loadrace loadregn loadscpt loadskil loadsndg loadsoun loadspel loadsscr loadstat
    loadweap aipackage effectlist spelllist variant variantimp loadtes3 cellref filter
    savedgame journalentry queststate locals globalscript player objectstate cellid cellstate globalmap
    inventorystate containerstate npcstate creaturestate dialoguestate statstate npcstats creaturestats
    weatherstate quickkeys fogstate spellstate activespells creaturelevliststate doorstate projectilestate debugprofile
    aisequence magiceffects custommarkerstate stolenitems transport animationstate controlsstate mappings readerscache
    infoorder timestamp formatversion landrecorddata selectiongroup dialoguecondition
    refnum
    )

add_component_dir (esmterrain
    gridsampling
    storage
    )

add_component_dir (esm4
    actor
    cellgrid
    common
    dialogue
    effect
    grid
    grouptype
    inventory
    lighting
    loadachr
    loadacti
    loadalch
    loadaloc
    loadammo
    loadanio
    loadappa
    loadarma
    loadarmo
    loadaspc
    loadbook
    loadbptd
    loadcell
    loadclas
    loadclfm
    loadclot
    loadcont
    loadcrea
    loaddial
    loaddobj
    loaddoor
    loadeyes
    loadflor
    loadflst
    loadfurn
    loadglob
    loadgmst
    loadgras
    loadgrup
    loadhair
    loadhdpt
    loadidle
    loadidlm
    loadimod
    loadinfo
    loadingr
    loadkeym
    loadland
    loadlgtm
    loadligh
    loadltex
    loadlvlc
    loadlvli
    loadlvln
    loadmato
    loadmisc
    loadmset
    loadmstt
    loadmusc
    loadnavi
    loadnavm
    loadnote
    loadnpc
    loadotft
    loadpack
    loadpgrd
    loadpgre
    loadpwat
    loadqust
    loadrace
    loadrefr
    loadregn
    loadroad
    loadsbsp
    loadscol
    loadscpt
    loadscrl
    loadsgst
    loadslgm
    loadsndr
    loadsoun
    loadstat
    loadtact
    loadterm
    loadtes4
    loadtree
    loadtxst
    loadweap
    loadwrld
    magiceffectid
    reader
    readerutils
    reference
    script
    typetraits
    vertex
)

add_component_dir (misc
    barrier budgetmeasurement color compression constants convert coordinateconverter display endianness float16 frameratelimiter
    guarded math mathutil messageformatparser notnullptr objectpool osgpluginchecker osguservalues progressreporter resourcehelpers
    rng strongtypedef thread timeconvert timer tuplehelpers tuplemeta utf8stream weakcache windows
    )

add_component_dir (misc/strings
    algorithm conversion format lower
    )

add_component_dir (stereo
    frustum multiview stereomanager types
    )

add_component_dir (debug
    debugging debuglog gldebug debugdraw writeflags
    )

add_definitions(-DMYGUI_DONT_USE_OBSOLETE=ON)
IF(NOT WIN32 AND NOT APPLE)
    add_definitions(-DGLOBAL_DATA_PATH="${GLOBAL_DATA_PATH}")
    add_definitions(-DGLOBAL_CONFIG_PATH="${GLOBAL_CONFIG_PATH}")
ENDIF()
add_component_dir (files
    linuxpath androidpath windowspath macospath fixedpath multidircollection collections configurationmanager
    constrainedfilestream memorystream hash configfileparser openfile constrainedfilestreambuf conversion
    istreamptr streamwithbuffer
    )

add_component_dir (compiler
    context controlparser errorhandler exception exprparser extensions fileparser generator
    lineparser literals locals output parser scanner scriptparser skipparser streamerrorhandler
    stringparser tokenloc nullerrorhandler opcodes extensions0 declarationparser
    quickfileparser discardparser junkparser
    )

add_component_dir (interpreter
    context controlopcodes genericopcodes installopcodes interpreter localopcodes mathopcodes
    miscopcodes opcodes program runtime types defines
    )

add_component_dir (translation
    translation
    )

add_component_dir (terrain
    storage world buffercache defs terraingrid material terraindrawable texturemanager chunkmanager compositemaprenderer
    quadtreeworld quadtreenode viewdata cellborder view heightcull
    )

add_component_dir (loadinglistener
    loadinglistener asynclistener
    )

add_component_dir (myguiplatform
    myguirendermanager myguidatamanager myguiplatform myguitexture myguiloglistener additivelayer scalinglayer
    )

add_component_dir (widgets
    box fontwrapper imagebutton tags list numericeditbox sharedstatebutton windowcaption widgets
    )

add_component_dir (fontloader
    fontloader
    )

add_component_dir (sdlutil
    events
    gl4es_init
    imagetosurface
    sdlcursormanager
    sdlgraphicswindow
    sdlinputwrapper
    sdlmappings
    sdlvideowrapper
    vsyncmode
    )

add_component_dir (version
    version
    )

add_component_dir (fallback
    fallback validate
    )

add_component_dir (lua_ui
    registerscriptsettings scriptsettings
    properties widget element util layers content alignment resources
    adapter text textedit window image container flex
    )
copy_resource_file("lua_ui/content.lua" "${OPENMW_RESOURCES_ROOT}" "resources/lua_libs/content.lua")


if(WIN32)
    add_component_dir (crashcatcher
        windows_crashcatcher
        windows_crashmonitor
        windows_crashshm
        windowscrashdumppathhelpers
    )
elseif(NOT ANDROID)
    add_component_dir (crashcatcher
        crashcatcher
    )
endif()

add_component_dir(detournavigator
    agentbounds
    areatype
    asyncnavmeshupdater
    bounds
    changetype
    collisionshapetype
    commulativeaabb
    dbrefgeometryobject
    debug
    exceptions
    findrandompointaroundcircle
    findsmoothpath
    flags
    generatenavmeshtile
    gettilespositions
    guardednavmeshcacheitem
    heightfieldshape
    makenavmesh
    navigator
    navigatorimpl
    navigatorstub
    navigatorutils
    navmeshcacheitem
    navmeshdata
    navmeshdb
    navmeshdbutils
    navmeshmanager
    navmeshtilescache
    navmeshtileview
    objectid
    objecttransform
    offmeshconnection
    offmeshconnectionsmanager
    preparednavmeshdata
    preparednavmeshdatatuple
    raycast
    recast
    recastallocutils
    recastcontext
    recastglobalallocator
    recastmesh
    recastmeshbuilder
    recastmeshobject
    recastmeshprovider
    recastmeshtiles
    recastparams
    recasttempallocator
    ref
    serialization
    settings
    settingsutils
    sharednavmeshcacheitem
    stats
    status
    tilebounds
    tilecachedrecastmeshmanager
    tileposition
    tilespositionsrange
    updateguard
    version
    waitconditiontype
    )

add_component_dir(loadinglistener
    reporter
    )

add_component_dir(serialization
    binaryreader
    binarywriter
    osgyaml
    sizeaccumulator
    )

add_component_dir(sqlite3
    db
    request
    statement
    transaction
)

add_component_dir(esmloader
    lessbyid
    load
    esmdata
)

add_component_dir(navmeshtool
    protocol
    )

add_component_dir(platform
    platform
    file
    )

if (WIN32)
    add_component_dir(platform
        file.win32
        )
elseif (UNIX)
    add_component_dir(platform
        file.posix
        )
else ()
    add_component_dir(platform
        file.stdio
        )
endif()

add_component_dir(testing
    expecterror
    util
)

if (USE_QT)
    add_component_qt_dir (contentselector
        model/modelitem model/esmfile
        model/contentmodel
        model/loadordererror
        view/combobox view/contentselector
        )

    add_component_qt_dir (config
        gamesettings
        launchersettings
        )

    add_component_qt_dir (l10n
        qttranslations
        )

    add_component_qt_dir (process
        processinvoker
        )

    add_component_qt_dir (misc
        helpviewer utf8qtextstream hash scalableicon
        )

    add_component_qt_dir (files
        qtconversion
        qtconfigpath
        )

    add_component_qt_dir (platform
        application
        )

    add_component_qt_dir (vfs
        qtconversion
        )

endif()

include_directories(${BULLET_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

find_package(SQLite3 REQUIRED)

add_library(components STATIC ${COMPONENT_FILES})

if (ANDROID)
    set_property(TARGET components PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_link_libraries(components
    ${COLLADA_DOM_LIBRARIES}

    # CMake's built-in OSG finder does not use pkgconfig, so we have to
    # manually ensure the order is correct for inter-library dependencies.
    # This only makes a difference with `-DOPENMW_USE_SYSTEM_OSG=ON -DOSG_STATIC=ON`.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21701
    ${OSGPARTICLE_LIBRARIES}
    ${OSGVIEWER_LIBRARIES}
    ${OSGSHADOW_LIBRARIES}
    ${OSGANIMATION_LIBRARIES}
    ${OSGGA_LIBRARIES}
    ${OSGTEXT_LIBRARIES}
    ${OSGDB_LIBRARIES}
    ${OSGUTIL_LIBRARIES}
    ${OSGSIM_LIBRARIES}
    ${OSG_LIBRARIES}
    ${OPENTHREADS_LIBRARIES}

    Boost::system
    Boost::program_options
    Boost::iostreams

    SDL2::SDL2
    ${OPENGL_gl_LIBRARY}
    ${MyGUI_LIBRARIES}
    ${LUA_LIBRARIES}
    LZ4::LZ4
    RecastNavigation::DebugUtils
    RecastNavigation::Detour
    RecastNavigation::Recast

    Base64
    SQLite::SQLite3
    smhasher
    ${ICU_LIBRARIES}
    ZLIB::ZLIB
    )

if(yaml-cpp_VERSION VERSION_GREATER_EQUAL 0.8.0)
    target_link_libraries(components yaml-cpp::yaml-cpp)
else()
    target_link_libraries(components yaml-cpp)
endif()

target_link_libraries(components ${BULLET_LIBRARIES})

if (USE_QT)
    add_library(components_qt STATIC ${COMPONENT_QT_FILES})
    target_link_libraries(components_qt components Qt::Widgets Qt::Core)

    if (BUILD_LAUNCHER OR BUILD_WIZARD)
        add_dependencies(components_qt qm-files)
    endif()

    if (BUILD_WITH_CODE_COVERAGE)
        target_compile_options(components_qt PRIVATE --coverage)
        target_link_libraries(components_qt gcov)
    endif()
endif()

if (OSG_STATIC AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED COMPONENTS Xinerama Xrandr)
    target_link_libraries(components ${CMAKE_DL_LIBS} X11::X11 X11::Xinerama X11::Xrandr)
    find_package(Fontconfig MODULE)
    if(Fontconfig_FOUND)
        target_link_libraries(components Fontconfig::Fontconfig)
    endif()
endif()

if (WIN32)
    target_link_libraries(components shlwapi)
endif()

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    target_link_libraries(components ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(components PRIVATE --coverage)
    target_link_libraries(components gcov)
endif()


# Make the variable accessible for other subdirectories
set(COMPONENT_FILES ${COMPONENT_FILES} PARENT_SCOPE)

target_compile_definitions(components PUBLIC BT_USE_DOUBLE_PRECISION)
target_compile_definitions(components PRIVATE OPENMW_DOC_BASEURL="${OPENMW_DOC_BASEURL}")

if(OSG_STATIC)
    unset(_osg_plugins_static_files)
    add_library(components_osg_plugins INTERFACE)
    foreach(_plugin ${USED_OSG_PLUGINS})
        string(TOUPPER ${_plugin} _plugin_uc)
        if(OPENMW_USE_SYSTEM_OSG)
            list(APPEND _osg_plugins_static_files ${${_plugin_uc}_LIBRARY})
        else()
            list(APPEND _osg_plugins_static_files $<TARGET_FILE:${${_plugin_uc}_LIBRARY}>)
            target_link_libraries(components_osg_plugins INTERFACE $<TARGET_PROPERTY:${${_plugin_uc}_LIBRARY},LINK_LIBRARIES>)
            add_dependencies(components_osg_plugins ${${_plugin_uc}_LIBRARY})
        endif()
    endforeach()
    # We use --whole-archive because OSG plugins use registration.
    get_whole_archive_options(_opts ${_osg_plugins_static_files})
    target_link_options(components_osg_plugins INTERFACE ${_opts})
    target_link_libraries(components components_osg_plugins)

    if(OPENMW_USE_SYSTEM_OSG)
        # OSG plugin pkgconfig files are missing these dependencies.
        # https://github.com/openscenegraph/OpenSceneGraph/issues/1052
        find_package(Freetype REQUIRED)
        find_package(JPEG REQUIRED)
        find_package(PNG REQUIRED)
        target_link_libraries(components Freetype::Freetype JPEG::JPEG PNG::PNG)
    endif()
endif(OSG_STATIC)

if(USE_QT)
    set_property(TARGET components_qt PROPERTY AUTOMOC ON)
    set_property(TARGET components_qt PROPERTY AUTOUIC ON)
    set_property(TARGET components_qt PROPERTY AUTOUIC_SEARCH_PATHS contentselector)
endif(USE_QT)

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(components PUBLIC
        <sol/sol.hpp>

        <osg/State>
        <osg/StateSet>
        <osg/Node>
        <osg/Drawable>
        <osg/Camera>

        <MyGUI_Widget.h>

        <algorithm>
        <filesystem>
        <fstream>
        <functional>
        <memory>
        <ostream>
        <string>
        <vector>
    )

    target_precompile_headers(components PRIVATE
        <osgViewer/Viewer>

        <boost/program_options/options_description.hpp>
    )
endif()
