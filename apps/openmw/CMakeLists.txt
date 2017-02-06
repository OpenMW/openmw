# local files
set(GAME
    main.cpp
    engine.cpp

    ${CMAKE_SOURCE_DIR}/files/tes3mp/tes3mp.rc
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
    renderbin actoranimation
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
    draganddrop timeadvancer jailscreen itemchargeview
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
    soundmanagerimp openal_output ffmpeg_decoder sound sound_buffer sound_decoder sound_output loudness movieaudiofactory
    )

add_openmw_dir (mwworld
    refdata worldimp scene globals class action nullaction actionteleport
    containerstore actiontalk actiontake manualref player cellvisitors failedaction
    cells localscripts customdata inventorystore ptr actionopen actionread
    actionequip timestamp actionalchemy cellstore actionapply actioneat
    store esmstore recordcmp fallback actionrepair actionsoulgem livecellref actiondoor
    contentloader esmloader actiontrap cellreflist cellref physicssystem weather projectilemanager
    cellpreloader
    )

add_openmw_dir (mwphysics
    physicssystem trace collisiontype actor convert
    )

add_openmw_dir (mwclass
    classes activator creature npc weapon armor potion apparatus book clothing container door
    ingredient creaturelevlist itemlevlist light lockpick misc probe repair static actor bodypart
    )

add_openmw_dir (mwmechanics
    mechanicsmanagerimp stat creaturestats magiceffects movement actorutil
    drawstate spells activespells npcstats aipackage aisequence aipursue alchemy aiwander aitravel aifollow aiavoiddoor
    aiescort aiactivate aicombat repair enchanting pathfinding pathgrid security spellsuccess spellcasting
    disease pickpocket levelledlist combat steering obstacle autocalcspell difficultyscaling aicombataction actor summoning
    character actors objects aistate coordinateconverter trading aiface
    )

add_openmw_dir (mwstate
    statemanagerimp charactermanager character
    )

add_openmw_dir (mwbase
    environment world scriptmanager dialoguemanager journal soundmanager mechanicsmanager
    inputmanager windowmanager statemanager
    )

add_openmw_dir (mwmp DedicatedPlayer WorldEvent LocalPlayer Networking Main GUIChat GUILogin GUIController PlayerMarkerCollection GUIDialogList WorldController)

# Main executable

if (NOT ANDROID)
    add_executable(tes3mp
        ${OPENMW_FILES}
        ${GAME} ${GAME_HEADER}
        ${APPLE_BUNDLE_RESOURCES}
    )
else ()
    add_library(tes3mp
        SHARED
        ${OPENMW_FILES}
        ${GAME} ${GAME_HEADER}
    )
endif ()

# Sound stuff - here so CMake doesn't stupidly recompile EVERYTHING
# when we change the backend.
include_directories(
    ${FFmpeg_INCLUDE_DIRS}
)

target_link_libraries(tes3mp
    ${OSG_LIBRARIES}
    ${OPENTHREADS_LIBRARIES}
    ${OSGPARTICLE_LIBRARIES}
    ${OSGUTIL_LIBRARIES}
    ${OSGDB_LIBRARIES}
    ${OSGVIEWER_LIBRARIES}
    ${OSGGA_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${OPENAL_LIBRARY}
    ${FFmpeg_LIBRARIES}
    ${MyGUI_LIBRARIES}
    ${SDL2_LIBRARY}
    "osg-ffmpeg-videoplayer"
    "oics"
    components
    ${RakNet_LIBRARY}
)

if (ANDROID)
    set (OSG_PLUGINS
        -Wl,--whole-archive
    )
    foreach(PLUGIN_NAME ${USED_OSG_PLUGINS})
        set(OSG_PLUGINS ${OSG_PLUGINS} ${OSG_PLUGINS_DIR}/lib${PLUGIN_NAME}.a)
    endforeach()

    set (OSG_PLUGINS
        ${OSG_PLUGINS} -Wl,--no-whole-archive
    )

    target_link_libraries(tes3mp
        EGL
        android
        log
        dl
        z
        ${OPENSCENEGRAPH_LIBRARIES}
        ${OSG_PLUGINS}
        jpeg
        gif
        png
    )
endif (ANDROID)

if (USE_SYSTEM_TINYXML)
    target_link_libraries(tes3mp ${TinyXML_LIBRARIES})
endif()

if (NOT UNIX)
target_link_libraries(tes3mp ${SDL2MAIN_LIBRARY})
endif()

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
target_link_libraries(tes3mp ${CMAKE_THREAD_LIBS_INIT})
endif()

if(APPLE)
    set(BUNDLE_RESOURCES_DIR "${APP_BUNDLE_DIR}/Contents/Resources")

    set(OPENMW_MYGUI_FILES_ROOT ${BUNDLE_RESOURCES_DIR})
    set(OPENMW_SHADERS_ROOT ${BUNDLE_RESOURCES_DIR})

    add_subdirectory(../../files/ ${CMAKE_CURRENT_BINARY_DIR}/files)

    configure_file("${OpenMW_BINARY_DIR}/settings-default.cfg" ${BUNDLE_RESOURCES_DIR} COPYONLY)
    configure_file("${OpenMW_BINARY_DIR}/openmw.cfg" ${BUNDLE_RESOURCES_DIR} COPYONLY)
    configure_file("${OpenMW_BINARY_DIR}/gamecontrollerdb.txt" ${BUNDLE_RESOURCES_DIR} COPYONLY)

    add_custom_command(TARGET openmw
        POST_BUILD
        COMMAND cp "${OpenMW_BINARY_DIR}/resources/version" "${BUNDLE_RESOURCES_DIR}/resources")

    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    target_link_libraries(tes3mp ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK})

    if (FFmpeg_FOUND)
        find_library(COREVIDEO_FRAMEWORK CoreVideo)
        find_library(VDA_FRAMEWORK VideoDecodeAcceleration)
        target_link_libraries(tes3mp z ${COREVIDEO_FRAMEWORK} ${VDA_FRAMEWORK})
    endif()
endif(APPLE)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(tes3mp gcov)
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
endif (MSVC)

if (WIN32)
    INSTALL(TARGETS tes3mp RUNTIME DESTINATION ".")
endif (WIN32)

