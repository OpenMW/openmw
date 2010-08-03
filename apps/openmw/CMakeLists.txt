project(OpenMW)

# local files

set(GAME
    main.cpp
    engine.cpp)
set(GAME_HEADER
    engine.hpp)
source_group(game FILES ${GAME} ${GAME_HEADER})

set(GAMEREND
    mwrender/mwscene.cpp
    mwrender/cellimp.cpp
    mwrender/interior.cpp
    mwrender/sky.cpp)
set(GAMEREND_HEADER
    mwrender/cell.hpp
    mwrender/cellimp.hpp
    mwrender/mwscene.hpp
    mwrender/interior.hpp
    mwrender/playerpos.hpp
    mwrender/sky.hpp)
source_group(apps\\openmw\\mwrender FILES ${GAMEREND} ${GAMEREND_HEADER})

set(GAMEINPUT
    mwinput/inputmanager.cpp
)
set(GAMEINPUT_HEADER
    mwinput/inputmanager.hpp)
source_group(apps\\openmw\\mwinput FILES ${GAMEINPUT} ${GAMEINPUT_HEADER})

set(GAMEGUI_HEADER
    mwgui/mw_layouts.hpp
    mwgui/window_manager.hpp
    mwgui/console.hpp
)
set(GAMEGUI
    mwgui/window_manager.cpp
    mwgui/console.cpp
)
source_group(apps\\openmw\\mwgui FILES ${GAMEGUI_HEADER} ${GAMEGUI})

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
    mwscript/extensions.cpp
    mwscript/globalscripts.cpp
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
    mwscript/extensions.hpp
    mwscript/globalscripts.hpp
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
    mwworld/classes.cpp
    mwworld/activator.cpp
    )
set(GAMEWORLD_HEADER
    mwworld/refdata.hpp
    mwworld/world.hpp
    mwworld/ptr.hpp
    mwworld/environment.hpp
    mwworld/globals.hpp
    mwworld/class.hpp
    mwworld/classes.hpp
    mwworld/activator.hpp
    )
source_group(apps\\openmw\\mwworld FILES ${GAMEWORLD} ${GAMEWORLD_HEADER})

set(GAMEMECHANICS
    mwmechanics/mechanicsmanager.cpp)
set(GAMEMECHANICS_HEADER
    mwmechanics/mechanicsmanager.hpp
    mwmechanics/stat.hpp
    mwmechanics/creaturestats.hpp
    )
source_group(apps\\openmw\\mwmechanics FILES ${GAMEMECHANICS} ${GAMEMECHANICS_HEADER})

set(OPENMW_CPP ${GAME} ${GAMEREND} ${GAMEINPUT} ${GAMESCRIPT} ${GAMESOUND} ${GAMEGUI} ${GAMEWORLD}
    ${GAMEMECHANICS}
    )
set(OPENMW_HEADER ${GAME_HEADER} ${GAMEREND_HEADER} ${GAMEINPUT_HEADER} ${GAMESCRIPT_HEADER}
    ${GAMESOUND_HEADER} ${GAMEGUI_HEADER} ${GAMEWORLD_HEADER} ${GAMEMECHANICS_HEADER}
    )

# Main executable
add_executable(openmw
    ${COMPONENTS} ${COMPONENTS_HEADER}
    ${OPENMW_LIBS} ${OPENMW_LIBS_HEADER}
    ${OPENMW_CPP} ${OPENMW_HEADER}
    ${APPLE_BUNDLE_RESOURCES}
    )

target_link_libraries(openmw
  ${OGRE_LIBRARIES}
  ${OIS_LIBRARIES}
  ${Boost_LIBRARIES}
  ${ICONV_LIBRARIES}
  caelum
  MyGUIEngine
  MyGUI.OgrePlatform
)

if (APPLE)
    find_library(CARBON_FRAMEWORK Carbon)
    target_link_libraries(openmw ${CARBON_FRAMEWORK})
endif (APPLE)
