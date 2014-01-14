
# config file
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp")

# local files
set(GAME
    main.cpp
    engine.cpp
)
if(NOT WIN32)
    set(GAME ${GAME} crashcatcher.cpp)
endif()
set(GAME_HEADER
    engine.hpp
    config.hpp
)
source_group(game FILES ${GAME} ${GAME_HEADER})

add_openmw_dir (mwrender
    renderingmanager debugging sky camera animation npcanimation creatureanimation activatoranimation
    actors objects renderinginterface localmap occlusionquery water shadows
    characterpreview globalmap videoplayer ripplesimulation refraction
    terrainstorage renderconst
    )

add_openmw_dir (mwinput
    inputmanagerimp
    )

add_openmw_dir (mwgui
    textinput widgets race class birth review windowmanagerimp console dialogue
    windowbase statswindow messagebox journalwindow charactercreation
    mapwindow windowpinnablebase tooltips scrollwindow bookwindow list
    formatting inventorywindow container hud countdialog tradewindow settingswindow
    confirmationdialog alchemywindow referenceinterface spellwindow mainmenu quickkeysmenu
    itemselection spellbuyingwindow loadingscreen levelupdialog waitdialog spellcreationdialog
    enchantingdialog trainingwindow travelwindow imagebutton exposedwindow cursor spellicons
    merchantrepair repair soulgemdialog companionwindow bookpage journalviewmodel journalbooks
    keywordsearch itemmodel containeritemmodel inventoryitemmodel sortfilteritemmodel itemview
    tradeitemmodel companionitemmodel pickpocketitemmodel fontloader controllers savegamedialog
    recharge
    )

add_openmw_dir (mwdialogue
    dialoguemanagerimp journalimp journalentry quest topic filter selectwrapper
    )

add_openmw_dir (mwscript
    locals scriptmanagerimp compilercontext interpretercontext cellextensions miscextensions
    guiextensions soundextensions skyextensions statsextensions containerextensions
    aiextensions controlextensions extensions globalscripts ref dialogueextensions
    animationextensions transformationextensions consoleextensions userextensions locals
    )

add_openmw_dir (mwsound
    soundmanagerimp openal_output audiere_decoder mpgsnd_decoder ffmpeg_decoder
    )

add_openmw_dir (mwworld
    refdata worldimp physicssystem scene globals class action nullaction actionteleport
    containerstore actiontalk actiontake manualref player cellfunctors failedaction
    cells localscripts customdata weather inventorystore ptr actionopen actionread
    actionequip timestamp actionalchemy cellstore actionapply actioneat
    esmstore store recordcmp fallback actionrepair actionsoulgem livecellref actiondoor
    contentloader esmloader omwloader actiontrap
    )

add_openmw_dir (mwclass
    classes activator creature npc weapon armor potion apparatus book clothing container door
    ingredient creaturelevlist itemlevlist light lockpick misc probe repair static
    )

add_openmw_dir (mwmechanics
    mechanicsmanagerimp stat character creaturestats magiceffects movement actors objects
    drawstate spells activespells npcstats aipackage aisequence alchemy aiwander aitravel aifollow
    aiescort aiactivate aicombat repair enchanting pathfinding security spellsuccess spellcasting
    disease pickpocket levelledlist
    )

add_openmw_dir (mwbase
    environment world scriptmanager dialoguemanager journal soundmanager mechanicsmanager
    inputmanager windowmanager
    )

# Main executable
set(BOOST_COMPONENTS system filesystem program_options thread wave)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

IF(OGRE_STATIC)
ADD_DEFINITIONS(-DENABLE_PLUGIN_OctreeSceneManager -DENABLE_PLUGIN_ParticleFX -DENABLE_PLUGIN_GL)
set(OGRE_STATIC_PLUGINS ${OGRE_Plugin_OctreeSceneManager_LIBRARIES} ${OGRE_Plugin_ParticleFX_LIBRARIES} ${OGRE_RenderSystem_GL_LIBRARIES})
IF(WIN32)
ADD_DEFINITIONS(-DENABLE_PLUGIN_Direct3D9)
list (APPEND OGRE_STATIC_PLUGINS ${OGRE_RenderSystem_Direct3D9_LIBRARIES})
ENDIF(WIN32)
IF (Cg_FOUND)
ADD_DEFINITIONS(-DENABLE_PLUGIN_CgProgramManager)
list (APPEND OGRE_STATIC_PLUGINS ${OGRE_Plugin_CgProgramManager_LIBRARIES} ${Cg_LIBRARIES})
ENDIF (Cg_FOUND)
ENDIF(OGRE_STATIC)

add_executable(openmw
    ${OPENMW_LIBS} ${OPENMW_LIBS_HEADER}
    ${OPENMW_FILES}
    ${GAME} ${GAME_HEADER}
    ${APPLE_BUNDLE_RESOURCES}
)

# Sound stuff - here so CMake doesn't stupidly recompile EVERYTHING
# when we change the backend.
include_directories(${SOUND_INPUT_INCLUDES} ${BULLET_INCLUDE_DIRS})
add_definitions(${SOUND_DEFINE})

target_link_libraries(openmw
    ${OGRE_LIBRARIES}
    ${OGRE_STATIC_PLUGINS}
    ${SHINY_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${SOUND_INPUT_LIBRARY}
    ${BULLET_LIBRARIES}
    ${MYGUI_LIBRARIES}
    ${SDL2_LIBRARY}
    ${MYGUI_PLATFORM_LIBRARIES}
    "oics"
    "sdl4ogre"
    components
)

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

if(DPKG_PROGRAM)
    INSTALL(TARGETS openmw RUNTIME DESTINATION games COMPONENT openmw)
endif(DPKG_PROGRAM)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw gcov)
endif()
