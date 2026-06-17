function(yojimbo_glob
    ret # The name of the return variable
    directories # The locations of the directories to glob from, as a list
    extensions # The extensions to glob for, as a list
    )

    set(sources "")
    foreach(dir IN LISTS directories)
        foreach(ext IN LISTS extensions)
            list(APPEND sources "${dir}/*.${ext}")
        endforeach()
    endforeach()

    file(GLOB source_files CONFIGURE_DEPENDS ${sources})

    set("${ret}" "${source_files}" PARENT_SCOPE)
endfunction()

# --- END FUNCTIONS ---

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
   "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo"
   )
    set(Yojimbo_MODE "DEBUG")
else()
    set(Yojimbo_MODE "RELEASE")
endif()

add_library(serialize INTERFACE)
target_include_directories(serialize INTERFACE "serialize")
target_compile_definitions(serialize
    INTERFACE "SERIALIZE_${Yojimbo_MODE}=1"
    )
target_sources(serialize INTERFACE "serialize/serialize.h")

add_library(reliable STATIC)
target_include_directories(reliable PUBLIC "reliable")
target_compile_definitions(reliable
    PUBLIC "RELIABLE_${Yojimbo_MODE}=1"
    )
target_sources(reliable
    PRIVATE "reliable/reliable.c"
    PUBLIC  "reliable/reliable.h"
    )

add_library(tlsf STATIC)
target_include_directories(tlsf PUBLIC "tlsf")
target_sources(tlsf
    PRIVATE "tlsf/tlsf.c"
    PUBLIC  "tlsf/tlsf.h"
    )

if(NOT WIN32)
    find_library(sodium NAMES sodium libsodium REQUIRED)
else()
    find_library(sodium NAMES sodium libsodium)
    if(NOT sodium)
        add_library(sodium STATIC)
        target_include_directories(sodium PUBLIC "sodium")

        yojimbo_glob(Yojimbo_SODIUM_SOURCES "sodium" "c")
        yojimbo_glob(Yojimbo_SODIUM_HEADERS "sodium" "h")
        target_sources(sodium
            PRIVATE "${Yojimbo_SODIUM_SOURCES}"
            PUBLIC  "${Yojimbo_SODIUM_HEADERS}"
            )
    endif()
endif()

add_library(netcode STATIC)
target_include_directories(netcode PUBLIC "netcode")
target_compile_definitions(netcode
    PUBLIC "NETCODE_${Yojimbo_MODE}=1"
    )
target_sources(netcode
    PRIVATE "netcode/netcode.c"
    PUBLIC  "netcode/netcode.h"
    )
target_link_libraries(netcode PRIVATE sodium)

add_library(yojimbo STATIC)
target_include_directories(yojimbo
    PRIVATE "." # yojimbo_allocator.cpp:32 " #include tlsf/tlsf.h "
    PUBLIC "include"
    )
target_compile_definitions(yojimbo
    PUBLIC "YOJIMBO_${Yojimbo_MODE}=1"
    )

yojimbo_glob(Yojimbo_SOURCES "source" "cpp")
yojimbo_glob(Yojimbo_HEADERS "include" "h")
target_sources(yojimbo
    PRIVATE "${Yojimbo_SOURCES}"
    PUBLIC  "${Yojimbo_HEADERS}"
    )

target_link_libraries(yojimbo
    PUBLIC
        # yojimbo.cpp
        sodium
        # yojimbo_client.cpp, yojimbo_server.cpp, yojimbo_platform.cpp
        netcode
        # yojimbo_base_client.cpp, yojimbo_base_server.cpp, yojimbo_client.cpp,
        # yojimbo_server.cpp, yojimbo_platform.cpp
        reliable
        # yojimbo_allocator.cpp
        tlsf
        # yojimbo.h, yojimbo_constants.h
        serialize
    )
