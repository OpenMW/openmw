project(openmw-mp)

option(BUILD_WITH_PAWN "Enable Pawn language" OFF)
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
if(BUILD_WITH_LUA)
    #set(Terra_ROOT ${CMAKE_SOURCE_DIR}/external/terra/)
    find_package(Terra REQUIRED)
    set(LuaScript_Sources
            Script/LangLua/LangLua.cpp
            Script/LangLua/LuaFunc.cpp)
    set(LuaScript_Headers ${Terra_INCLUDES} ${CMAKE_SOURCE_DIR}/extern/LuaBridge ${CMAKE_SOURCE_DIR}/extern/LuaBridge/detail
            Script/LangLua/LangLua.hpp)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_LUA")
    include_directories(${Terra_INCLUDES} ${CMAKE_SOURCE_DIR}/extern/LuaBridge)
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
    Utils.cpp
    Log.cpp
    Script/Script.cpp Script/ScriptFunction.cpp
    Script/ScriptFunctions.cpp
    Script/Functions/Translocations.cpp Script/Functions/Stats.cpp Script/Functions/Items.cpp
    Script/Functions/Timer.cpp Script/Functions/Chat.cpp Script/Functions/GUI.cpp
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
source_group(openmw-mp FILES ${SERVER} ${SERVER_HEADER})

include_directories("./")

# Main executable

add_executable(openmw-mp
${SERVER_FILES}
${SERVER} ${SERVER_HEADER}
${APPLE_BUNDLE_RESOURCES}
)
add_definitions(-std=gnu++14 -Wno-ignored-qualifiers)

target_link_libraries(openmw-mp
    #${Boost_SYSTEM_LIBRARY}
    #${Boost_THREAD_LIBRARY}
    #${Boost_FILESYSTEM_LIBRARY}
    #${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${RakNet_LIBRARY}
    components
    ${Terra_LIBRARY}
    ${Pawn_LIBRARY}
)

if (UNIX)
    target_link_libraries(openmw-mp dl)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if(NOT APPLE)
        target_link_libraries(openmw-mp ${CMAKE_THREAD_LIBS_INIT})
    endif(NOT APPLE)
endif(UNIX)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-mp gcov)
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
    add_definitions("-D_USE_MATH_DEFINES")
endif (MSVC)
