project(OpenMW)

# config file

configure_file ("${OpenMW_SOURCE_DIR}/config.hpp.cmake" "${OpenMW_SOURCE_DIR}/config.hpp")

# local files

set(GAME
    main.cpp
    engine.cpp
    )
set(GAME_HEADER
    engine.hpp
    config.hpp)
source_group(game FILES ${GAME} ${GAME_HEADER})

add_openmw_dir (mwrender
    rendering_manager mwscene cellimp interior exterior sky player
    )

add_openmw_dir (mwinput
    inputmanager
    )

add_openmw_dir (mwgui
    layouts text_input widgets race class birth review window_manager console dialogue
    dialogue_history window_base stats_window messagebox
    )

add_openmw_dir (mwdialogue
    dialoguemanager journal journalentry quest topic
    )

add_openmw_dir (mwscript
    locals scriptmanager compilercontext interpretercontext cellextensions miscextensions
    guiextensions soundextensions skyextensions statsextensions containerextensions
    aiextensions controlextensions extensions globalscripts ref dialogueextensions
    )

add_openmw_dir (mwsound
    soundmanager
    )

add_openmw_dir (mwworld
    refdata world physicssystem scene environment globals class action nullaction actionteleport
    containerstore actiontalk actiontake containerstore manualref containerutil player cellfunctors
    cells localscripts
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
    ${COMPONENTS} ${COMPONENTS_HEADER}
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
  ${OIS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${SOUND_INPUT_LIBRARY}
  ${BULLET_LIBRARIES}
  caelum
  MyGUIEngine
  MyGUIOgrePlatform
)

if (APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(openmw ${CARBON_FRAMEWORK})
    install(TARGETS openmw
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ../MacOS
        COMPONENT Runtime)
endif (APPLE)

if(DPKG_PROGRAM)
    INSTALL(TARGETS openmw RUNTIME DESTINATION games COMPONENT openmw)
endif()
