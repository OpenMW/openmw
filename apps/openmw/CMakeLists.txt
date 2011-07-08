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

set(GAMEREND
    mwrender/mwscene.cpp
    mwrender/cellimp.cpp
    mwrender/interior.cpp
    mwrender/exterior.cpp
    mwrender/sky.cpp
    mwrender/player.cpp
    )
set(GAMEREND_HEADER
    mwrender/cell.hpp
    mwrender/cellimp.hpp
    mwrender/mwscene.hpp
    mwrender/interior.hpp
    mwrender/exterior.hpp
    mwrender/sky.hpp
    mwrender/player.hpp
    )
source_group(apps\\openmw\\mwrender FILES ${GAMEREND} ${GAMEREND_HEADER})

set(GAMEINPUT
    mwinput/inputmanager.cpp
)
set(GAMEINPUT_HEADER
    mwinput/inputmanager.hpp)
source_group(apps\\openmw\\mwinput FILES ${GAMEINPUT} ${GAMEINPUT_HEADER})

set(GAMEGUI_HEADER
    mwgui/layouts.hpp
    mwgui/text_input.hpp
    mwgui/widgets.hpp
    mwgui/race.hpp
    mwgui/class.hpp
    mwgui/birth.hpp
    mwgui/review.hpp
    mwgui/window_manager.hpp
    mwgui/console.hpp
    mwgui/dialogue.hpp
    mwgui/dialogue_history.hpp
    mwgui/window_base.hpp
    mwgui/stats_window.hpp
    mwgui/messagebox.hpp
)
set(GAMEGUI
    mwgui/window_manager.cpp
    mwgui/layouts.cpp
    mwgui/console.cpp
    mwgui/text_input.cpp
    mwgui/widgets.cpp
    mwgui/race.cpp
    mwgui/birth.cpp
    mwgui/class.cpp
    mwgui/review.cpp
    mwgui/dialogue.cpp
    mwgui/dialogue_history.cpp
    mwgui/window_base.cpp
    mwgui/stats_window.cpp
    mwgui/messagebox.cpp
)
source_group(apps\\openmw\\mwgui FILES ${GAMEGUI_HEADER} ${GAMEGUI})

set(GAMEDIALOGUE_HEADER
    mwdialogue/dialoguemanager.hpp
    mwdialogue/journal.hpp
    mwdialogue/journalentry.hpp
    mwdialogue/quest.hpp
    mwdialogue/topic.hpp
)
set(GAMEDIALOGUE
    mwdialogue/dialoguemanager.cpp
    mwdialogue/journal.cpp
    mwdialogue/journalentry.cpp
    mwdialogue/quest.cpp
    mwdialogue/topic.cpp
)
source_group(apps\\openmw\\mwdialogue FILES ${GAMEDIALOGUE_HEADER} ${GAMEDIALOGUE})

set(GAMESCRIPT
    mwscript/scriptmanager.cpp
    mwscript/compilercontext.cpp
    mwscript/interpretercontext.cpp
    mwscript/cellextensions.cpp
    mwscript/miscextensions.cpp
    mwscript/guiextensions.cpp
    mwscript/soundextensions.cpp
    mwscript/skyextensions.cpp
    mwscript/statsextensions.cpp
    mwscript/containerextensions.cpp
    mwscript/aiextensions.cpp
    mwscript/controlextensions.cpp
    mwscript/extensions.cpp
    mwscript/globalscripts.cpp
    mwscript/dialogueextensions.cpp
    )
set(GAMESCRIPT_HEADER
    mwscript/locals.hpp
    mwscript/scriptmanager.hpp
    mwscript/compilercontext.hpp
    mwscript/interpretercontext.hpp
    mwscript/cellextensions.hpp
    mwscript/miscextensions.hpp
    mwscript/guiextensions.hpp
    mwscript/soundextensions.hpp
    mwscript/skyextensions.hpp
    mwscript/statsextensions.hpp
    mwscript/containerextensions.hpp
    mwscript/aiextensions.hpp
    mwscript/controlextensions.hpp
    mwscript/extensions.hpp
    mwscript/globalscripts.hpp
    mwscript/ref.hpp
    mwscript/dialogueextensions.hpp
    )
source_group(apps\\openmw\\mwscript FILES ${GAMESCRIPT} ${GAMESCRIPT_HEADER})

set(GAMESOUND
    mwsound/soundmanager.cpp)
set(GAMESOUND_HEADER
    mwsound/soundmanager.hpp)
source_group(apps\\openmw\\mwsound FILES ${GAMESOUND} ${GAMESOUND_HEADER})

set(GAMEWORLD
    mwworld/world.cpp
    mwworld/globals.cpp
    mwworld/class.cpp
    mwworld/actionteleport.cpp
    mwworld/actiontalk.cpp
    mwworld/actiontake.cpp
    mwworld/containerutil.cpp
    mwworld/player.cpp
    mwworld/doingphysics.cpp
    )
set(GAMEWORLD_HEADER
    mwworld/refdata.hpp
    mwworld/world.hpp
    mwworld/ptr.hpp
    mwworld/environment.hpp
    mwworld/globals.hpp
    mwworld/class.hpp
    mwworld/action.hpp
    mwworld/nullaction.hpp
    mwworld/actionteleport.hpp
    mwworld/containerstore.hpp
    mwworld/actiontalk.hpp
    mwworld/actiontake.hpp
    mwworld/containerstore.hpp
    mwworld/manualref.hpp
    mwworld/containerutil.hpp
    mwworld/player.hpp
    mwworld/doingphysics.hpp
    mwworld/cellfunctors.hpp
    )
source_group(apps\\openmw\\mwworld FILES ${GAMEWORLD} ${GAMEWORLD_HEADER})

set(GAMECLASS
    mwclass/classes.cpp
    mwclass/activator.cpp
    mwclass/creature.cpp
    mwclass/npc.cpp
    mwclass/weapon.cpp
    mwclass/armor.cpp
    mwclass/potion.cpp
    mwclass/apparatus.cpp
    mwclass/book.cpp
    mwclass/clothing.cpp
    mwclass/container.cpp
    mwclass/door.cpp
    mwclass/ingredient.cpp
    mwclass/creaturelevlist.cpp
    mwclass/itemlevlist.cpp
    mwclass/light.cpp
    mwclass/lockpick.cpp
    mwclass/misc.cpp
    mwclass/probe.cpp
    mwclass/repair.cpp
    mwclass/static.cpp
    )
set(GAMECLASS_HEADER
    mwclass/classes.hpp
    mwclass/activator.hpp
    mwclass/creature.hpp
    mwclass/npc.hpp
    mwclass/weapon.hpp
    mwclass/armor.hpp
    mwclass/potion.hpp
    mwclass/apparatus.hpp
    mwclass/book.hpp
    mwclass/clothing.hpp
    mwclass/container.hpp
    mwclass/door.hpp
    mwclass/ingredient.hpp
    mwclass/creaturelevlist.hpp
    mwclass/itemlevlist.hpp
    mwclass/light.hpp
    mwclass/lockpick.hpp
    mwclass/misc.hpp
    mwclass/probe.hpp
    mwclass/repair.hpp
    mwclass/static.hpp
    mwclass/containerutil.hpp
    )
source_group(apps\\openmw\\mwclass FILES ${GAMECLASS} ${GAMECLASS_HEADER})

set(GAMEMECHANICS
    mwmechanics/mechanicsmanager.cpp
    mwmechanics/magiceffects.cpp
    )
set(GAMEMECHANICS_HEADER
    mwmechanics/mechanicsmanager.hpp
    mwmechanics/stat.hpp
    mwmechanics/creaturestats.hpp
    mwmechanics/magiceffects.hpp
    mwmechanics/movement.hpp
    )
source_group(apps\\openmw\\mwmechanics FILES ${GAMEMECHANICS} ${GAMEMECHANICS_HEADER})

set(OPENMW_CPP ${GAME} ${GAMEREND} ${GAMEINPUT} ${GAMESCRIPT} ${GAMESOUND} ${GAMEGUI} ${GAMEWORLD}
    ${GAMECLASS} ${GAMEMECHANICS} ${GAMEDIALOGUE}
    )
set(OPENMW_HEADER ${GAME_HEADER} ${GAMEREND_HEADER} ${GAMEINPUT_HEADER} ${GAMESCRIPT_HEADER}
    ${GAMESOUND_HEADER} ${GAMEGUI_HEADER} ${GAMEWORLD_HEADER}  ${GAMECLASS_HEADER}
    ${GAMEMECHANICS_HEADER} ${GAMEDIALOG_HEADERUE}
    )

# Main executable
add_executable(openmw
    ${COMPONENTS} ${COMPONENTS_HEADER}
    ${OPENMW_LIBS} ${OPENMW_LIBS_HEADER}
    ${OPENMW_CPP} ${OPENMW_HEADER}
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
