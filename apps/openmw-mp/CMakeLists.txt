project(tes3mp-server)

option(BUILD_WITH_PAWN "Enable Pawn language" OFF)
option(ENABLE_BREAKPAD "Enable Google Breakpad for Crash reporting" OFF)

if(ENABLE_BREAKPAD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_BREAKPAD")
    if (UNIX)
        set(Breakpad_Headers "${CMAKE_SOURCE_DIR}/extern/breakpad/src/client/linux")
        set(Breakpad_Library "${CMAKE_SOURCE_DIR}/extern/breakpad/src/client/linux/libbreakpad_client.a")
    elseif(WIN32)
        set(Breakpad_Headers "${CMAKE_SOURCE_DIR}/extern/breakpad/src/client/windows")
        set(Breakpad_Library "-lbreakpad_client")
    endif (UNIX)
    include_directories(${CMAKE_SOURCE_DIR}/extern/breakpad/src ${Breakpad_Headers})
endif(ENABLE_BREAKPAD)

if(BUILD_WITH_PAWN)

    add_subdirectory(amx)

    #set(Pawn_ROOT ${CMAKE_SOURCE_DIR}/external/pawn/)
    set(Pawn_INCLUDES ${Pawn_ROOT}/include)
    set(Pawn_LIBRARY ${Pawn_ROOT}/lib/libamx.a)
    set(PawnScript_Sources
            Script/LangPawn/LangPAWN.cpp
            Script/LangPawn/PawnFunc.cpp)
    set(PawnScript_Headers ${Pawn_INCLUDES}
            Script/LangPawn/LangPAWN.hpp
            )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_PAWN -DPAWN_CELL_SIZE=64")
    #include_directories(${Pawn_INCLUDES})
    include_directories("./amx/linux")
endif(BUILD_WITH_PAWN)

option(BUILD_WITH_LUA "Enable Terra/Lua language" ON)
option(FORCE_LUA "Use Lua instead Terra" OFF)
if(BUILD_WITH_LUA)
    #set(Terra_ROOT ${CMAKE_SOURCE_DIR}/external/terra/)
    if(WIN32 OR FORCE_LUA)
        find_package(Lua51 REQUIRED)
        MESSAGE(STATUS "Found LUA_LIBRARY: ${LUA_LIBRARY}")
        MESSAGE(STATUS "Found LUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}")
    else()
        find_package(Terra REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_TERRA")
    endif()
    set(LuaScript_Sources
            Script/LangLua/LangLua.cpp
            Script/LangLua/LuaFunc.cpp)
    set(LuaScript_Headers ${Terra_INCLUDES} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/extern/LuaBridge ${CMAKE_SOURCE_DIR}/extern/LuaBridge/detail
            Script/LangLua/LangLua.hpp)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_LUA")
    include_directories(${Terra_INCLUDES} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/extern/LuaBridge)
endif(BUILD_WITH_LUA)

set(NativeScript_Sources
        Script/LangNative/LangNative.cpp
        )
set(NativeScript_Headers
        Script/LangNative/LangNative.hpp
        )

# local files
set(SERVER
    main.cpp
    Player.cpp
    Networking.cpp
    MasterClient.cpp
    Cell.cpp
    Utils.cpp
    Script/Script.cpp Script/ScriptFunction.cpp
    Script/ScriptFunctions.cpp

    Script/Functions/Actors.cpp Script/Functions/World.cpp Script/Functions/Miscellaneous.cpp

    Script/Functions/CharClass.cpp Script/Functions/Chat.cpp Script/Functions/GUI.cpp
    Script/Functions/Items.cpp Script/Functions/Quests.cpp Script/Functions/Stats.cpp
    Script/Functions/Spells.cpp Script/Functions/Timer.cpp Script/Functions/Positions.cpp
    Script/Functions/Cells.cpp

    ProcessorInitializer.cpp PlayerProcessor.cpp ActorProcessor.cpp WorldProcessor.cpp

    Script/API/TimerAPI.cpp Script/API/PublicFnAPI.cpp
        ${PawnScript_Sources}
        ${LuaScript_Sources}
        ${NativeScript_Sources}

)

set(SERVER_HEADER
        Script/Types.hpp Script/Script.hpp Script/SystemInterface.hpp
        Script/ScriptFunction.hpp Script/Platform.hpp Script/Language.hpp
        Script/ScriptFunctions.hpp Script/API/TimerAPI.hpp Script/API/PublicFnAPI.hpp
        ${PawnScript_Headers}
        ${LuaScript_Headers}
        ${NativeScript_Headers}
)
source_group(tes3mp-server FILES ${SERVER} ${SERVER_HEADER})

set(PROCESSORS_ACTOR
        processors/actor/ProcessorActorAnimFlags.hpp processors/actor/ProcessorActorAnimPlay.hpp
        processors/actor/ProcessorActorAttack.hpp processors/actor/ProcessorActorAuthority.hpp
        processors/actor/ProcessorActorCellChange.hpp processors/actor/ProcessorActorList.hpp
        processors/actor/ProcessorActorPosition.hpp processors/actor/ProcessorActorSpeech.hpp
        processors/actor/ProcessorActorStatsDynamic.hpp processors/actor/ProcessorActorTest.hpp
        )

source_group(tes3mp-server\\processors\\actor FILES ${PROCESSORS_ACTOR})

set(PROCESSORS_PLAYER
        processors/player/ProcessorChatMsg.hpp processors/player/ProcessorGUIMessageBox.hpp
        processors/player/ProcessorPlayerAnimFlags.hpp processors/player/ProcessorPlayerAttack.hpp
        processors/player/ProcessorPlayerAttribute.hpp processors/player/ProcessorPlayerBounty.hpp
        processors/player/ProcessorPlayerCellChange.hpp processors/player/ProcessorPlayerCellState.hpp
        processors/player/ProcessorPlayerCharClass.hpp processors/player/ProcessorPlayerCharGen.hpp
        processors/player/ProcessorPlayerDeath.hpp processors/player/ProcessorPlayerEquipment.hpp
        processors/player/ProcessorPlayerInventory.hpp processors/player/ProcessorPlayerJournal.hpp
        processors/player/ProcessorPlayerLevel.hpp processors/player/ProcessorPlayerPos.hpp
        processors/player/ProcessorPlayerResurrect.hpp processors/player/ProcessorPlayerSkill.hpp
        processors/player/ProcessorPlayerSpellbook.hpp processors/player/ProcessorPlayerStatsDynamic.hpp
        )

source_group(tes3mp-server\\processors\\player FILES ${PROCESSORS_PLAYER})

set(PROCESSORS_WORLD
        processors/world/ProcessorContainer.hpp processors/world/ProcessorDoorState.hpp
        processors/world/ProcessorMusicPlay.hpp processors/world/ProcessorObjectAnimPlay.hpp
        processors/world/ProcessorObjectDelete.hpp processors/world/ProcessorObjectLock.hpp
        processors/world/ProcessorObjectMove.hpp processors/world/ProcessorObjectPlace.hpp
        processors/world/ProcessorObjectRotate.hpp processors/world/ProcessorObjectScale.hpp
        processors/world/ProcessorObjectUnlock.hpp processors/world/ProcessorScriptGlobalShort.hpp
        processors/world/ProcessorScriptLocalFloat.hpp processors/world/ProcessorScriptLocalShort.hpp
        processors/world/ProcessorScriptMemberShort.hpp processors/world/ProcessorVideoPlay.hpp
        )

source_group(tes3mp-server\\processors\\world FILES ${PROCESSORS_WORLD})

include_directories("./")

# Main executable

add_executable(tes3mp-server
        ${SERVER} ${SERVER_HEADER}
        ${PROCESSORS_ACTOR} ${PROCESSORS_PLAYER} ${PROCESSORS_WORLD}
        ${APPLE_BUNDLE_RESOURCES}
        )
add_definitions(-std=gnu++14 -Wno-ignored-qualifiers)

target_link_libraries(tes3mp-server
    #${Boost_SYSTEM_LIBRARY}
    #${Boost_THREAD_LIBRARY}
    #${Boost_FILESYSTEM_LIBRARY}
    #${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${RakNet_LIBRARY}
    components
    ${Terra_LIBRARY}
    ${LUA_LIBRARIES}
    ${Pawn_LIBRARY}
    ${Breakpad_Library}
)

if (UNIX)
    target_link_libraries(tes3mp-server dl)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if(NOT APPLE)
        target_link_libraries(tes3mp-server ${CMAKE_THREAD_LIBS_INIT})
    endif(NOT APPLE)
endif(UNIX)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(tes3mp-server gcov)
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
    add_definitions("-D_USE_MATH_DEFINES")
endif (MSVC)
