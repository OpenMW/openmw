# local files
set(GAME
    main.cpp
    engine.cpp

    ${CMAKE_SOURCE_DIR}/files/windows/openmw.rc
)

if (ANDROID)
    set(GAME ${GAME} android_commandLine.cpp)
    set(GAME ${GAME} android_main.c)
endif()

if(NOT WIN32 AND NOT ANDROID)
    set(GAME ${GAME} crashcatcher.cpp)
endif()
set(GAME_HEADER
    engine.hpp
)
source_group(game FILES ${GAME} ${GAME_HEADER})

add_openmw_dir (mwrender
    actors objects renderingmanager animation rotatecontroller sky npcanimation vismask
    creatureanimation effectmanager util renderinginterface pathgrid rendermode weaponanimation
    bulletdebugdraw globalmap characterpreview camera localmap water terrainstorage ripplesimulation
    renderbin
    )

add_openmw_dir (mwinput
    inputmanagerimp
    )

add_openmw_dir (mwgui
    layout textinput widgets race class birth review windowmanagerimp console dialogue
    windowbase statswindow messagebox journalwindow charactercreation
    mapwindow windowpinnablebase tooltips scrollwindow bookwindow
    formatting inventorywindow container hud countdialog tradewindow settingswindow
    confirmationdialog alchemywindow referenceinterface spellwindow mainmenu quickkeysmenu
    itemselection spellbuyingwindow loadingscreen levelupdialog waitdialog spellcreationdialog
    enchantingdialog trainingwindow travelwindow exposedwindow cursor spellicons
    merchantrepair repair soulgemdialog companionwindow bookpage journalviewmodel journalbooks
    itemmodel containeritemmodel inventoryitemmodel sortfilteritemmodel itemview
    tradeitemmodel companionitemmodel pickpocketitemmodel controllers savegamedialog
    recharge mode videowidget backgroundimage itemwidget screenfader debugwindow spellmodel spellview
    draganddrop timeadvancer jailscreen
    )

add_openmw_dir (mwdialogue
    dialoguemanagerimp journalimp journalentry quest topic filter selectwrapper hypertextparser keywordsearch scripttest
    )

add_openmw_dir (mwscript
    locals scriptmanagerimp compilercontext interpretercontext cellextensions miscextensions
    guiextensions soundextensions skyextensions statsextensions containerextensions
    aiextensions controlextensions extensions globalscripts ref dialogueextensions
    animationextensions transformationextensions consoleextensions userextensions
    )

add_openmw_dir (mwsound
    soundmanagerimp openal_output ffmpeg_decoder sound sound_decoder sound_output loudness movieaudiofactory
    )

add_openmw_dir (mwworld
    refdata worldimp scene globals class action nullaction actionteleport
    containerstore actiontalk actiontake manualref player cellfunctors failedaction
    cells localscripts customdata inventorystore ptr actionopen actionread
    actionequip timestamp actionalchemy cellstore actionapply actioneat
    store esmstore recordcmp fallback actionrepair actionsoulgem livecellref actiondoor
    contentloader esmloader actiontrap cellreflist cellref physicssystem weather projectilemanager
    )

add_openmw_dir (mwphysics
    physicssystem trace collisiontype actor convert
    )

add_openmw_dir (mwclass
    classes activator creature npc weapon armor potion apparatus book clothing container door
    ingredient creaturelevlist itemlevlist light lockpick misc probe repair static actor
    )

add_openmw_dir (mwmechanics
    mechanicsmanagerimp stat creaturestats magiceffects movement actorutil
    drawstate spells activespells npcstats aipackage aisequence aipursue alchemy aiwander aitravel aifollow aiavoiddoor
    aiescort aiactivate aicombat repair enchanting pathfinding pathgrid security spellsuccess spellcasting
    disease pickpocket levelledlist combat steering obstacle autocalcspell difficultyscaling aicombataction actor summoning
    character actors objects aistate coordinateconverter
    )

add_openmw_dir (mwstate
    statemanagerimp charactermanager character
    )

add_openmw_dir (mwbase
    environment world scriptmanager dialoguemanager journal soundmanager mechanicsmanager
    inputmanager windowmanager statemanager
    )

# Main executable

if (NOT ANDROID)
    add_executable(openmw
        ${OPENMW_FILES}
        ${GAME} ${GAME_HEADER}
        ${APPLE_BUNDLE_RESOURCES}
    )
else ()
    add_library(openmw
        SHARED
        ${OPENMW_FILES}
        ${GAME} ${GAME_HEADER}
    )
endif ()

# Sound stuff - here so CMake doesn't stupidly recompile EVERYTHING
# when we change the backend.
include_directories(
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(openmw
    ${OPENSCENEGRAPH_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${OPENAL_LIBRARY}
    ${FFMPEG_LIBRARIES}
    ${BULLET_LIBRARIES}
    ${MYGUI_LIBRARIES}
    ${SDL2_LIBRARY}
    "osg-ffmpeg-videoplayer"
    "oics"
    components
)

if (ANDROID)
    set (OSG_PLUGINS
        -Wl,--whole-archive
        ${OSG_PLUGINS_DIR}/libosgdb_dds.a
        ${OSG_PLUGINS_DIR}/libosgdb_bmp.a
        ${OSG_PLUGINS_DIR}/libosgdb_tga.a
        ${OSG_PLUGINS_DIR}/libosgdb_gif.a
        ${OSG_PLUGINS_DIR}/libosgdb_jpeg.a
        ${OSG_PLUGINS_DIR}/libosgdb_png.a
        -Wl,--no-whole-archive
    )
    target_link_libraries(openmw
        EGL
        android
        log
        dl
        MyGUIEngineStatic
        BulletCollision
        LinearMath
        z
        osg
        osgDB
        osgAnimation
        osgText
        osgUtil
        osgShadow
        ${OPENSCENEGRAPH_LIBRARIES}
        ${OSG_PLUGINS}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        jpeg
	gif
	png
    )
endif (ANDROID)

if (OPENGLES)
    target_link_libraries(openmw
        ${OPENGLES_gl_LIBRARY}
        ${OPENGLES2_gl_LIBRARY}
    )
endif (OPENGLES)

if (USE_SYSTEM_TINYXML)
    target_link_libraries(openmw ${TINYXML_LIBRARIES})
endif()

if (NOT UNIX)
target_link_libraries(openmw ${SDL2MAIN_LIBRARY})
endif()

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
target_link_libraries(openmw ${CMAKE_THREAD_LIBS_INIT})
endif()

if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(openmw ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK})

    if (FFMPEG_FOUND)
        find_library(COREVIDEO_FRAMEWORK CoreVideo)
        find_library(VDA_FRAMEWORK VideoDecodeAcceleration)
        target_link_libraries(openmw ${COREVIDEO_FRAMEWORK} ${VDA_FRAMEWORK})
    endif()
endif(APPLE)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw gcov)
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
    add_definitions("-D_USE_MATH_DEFINES")
endif (MSVC)
