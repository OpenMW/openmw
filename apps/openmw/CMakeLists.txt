
# config file
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp")

# local files
set(GAME
    main.cpp
    engine.cpp
)
set(GAME_HEADER
    engine.hpp
    config.hpp
)
source_group(game FILES ${GAME} ${GAME_HEADER})

add_openmw_dir (mwrender
    renderingmanager debugging sky terrain terrainmaterial player animation npcanimation creatureanimation actors objects renderinginterface
    )

add_openmw_dir (mwinput
    inputmanager
    )

add_openmw_dir (mwgui
    layouts text_input widgets race class birth review window_manager console dialogue
    dialogue_history window_base stats_window messagebox journalwindow charactercreation
    )

add_openmw_dir (mwdialogue
    dialoguemanager journal journalentry quest topic
    )

add_openmw_dir (mwscript
    locals scriptmanager compilercontext interpretercontext cellextensions miscextensions
    guiextensions soundextensions skyextensions statsextensions containerextensions
    aiextensions controlextensions extensions globalscripts ref dialogueextensions
    animationextensions
    )

add_openmw_dir (mwsound
    soundmanager
    )

add_openmw_dir (mwworld
    refdata world physicssystem scene environment globals class action nullaction actionteleport
    containerstore actiontalk actiontake manualref player cellfunctors
    cells localscripts customdata weather
    )

add_openmw_dir (mwclass
    classes activator creature npc weapon armor potion apparatus book clothing container door
    ingredient creaturelevlist itemlevlist light lockpick misc probe repair static
    )

add_openmw_dir (mwmechanics
    mechanicsmanager stat creaturestats magiceffects movement
    )

# Main executable
add_executable(openmw
    ${OPENMW_LIBS} ${OPENMW_LIBS_HEADER}
    ${COMPONENT_FILES}
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
    ${OGRE_Terrain_LIBRARY}
    ${OIS_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${SOUND_INPUT_LIBRARY}
    ${BULLET_LIBRARIES}
    components
    MyGUIEngine
    MyGUIOgrePlatform
)

if(APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(openmw ${CARBON_FRAMEWORK})
endif(APPLE)

if(DPKG_PROGRAM)
    INSTALL(TARGETS openmw RUNTIME DESTINATION games COMPONENT openmw)
endif(DPKG_PROGRAM)
