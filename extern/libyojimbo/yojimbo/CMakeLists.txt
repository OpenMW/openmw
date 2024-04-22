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

function(yojimbo_adlib_interface
    lib_name # The name of the library target to create (set in parent scope)
    lib_type # must be INTERFACE
    itf_dirs # The locations of the include directories, as a list
    itf_exts # The file extensions of the library's header code.
    )

    add_library("${lib_name}" INTERFACE)
    target_include_directories("${lib_name}" INTERFACE ${itf_dirs})

    yojimbo_glob(headers "${itf_dirs}" "${itf_exts}")
    target_sources("${lib_name}" INTERFACE "${headers}")

    set("${lib_name}" "${lib_name}" PARENT_SCOPE)
endfunction()

function(yojimbo_adlib
    lib_name # The name of the library target to create (set in parent scope)
    lib_type # The type of library to create (STATIC, SHARED)
    src_dirs # The locations of the source directories, as a list
    src_exts # The file extension of the library's source code.
    hdr_dirs # The locations of the include directories, as a list
    hdr_exts # The file extension of the library's header code.
    )

    add_library("${lib_name}" "${lib_type}")

    target_include_directories("${lib_name}" PUBLIC ${hdr_dirs})

    yojimbo_glob(sources "${src_dirs}" "${src_exts}")
    yojimbo_glob(headers "${hdr_dirs}" "${hdr_exts}")

    target_sources("${lib_name}" PRIVATE "${sources}" PUBLIC "${headers}")

    set("${lib_name}" "${lib_name}" PARENT_SCOPE)
endfunction()

# --- END FUNCTIONS ---

set(Yojimbo_SERIALIZE_INCLUDE_DIR "serialize")
set(Yojimbo_NETCODE_INCLUDE_DIR "netcode")
set(Yojimbo_RELIABLE_INCLUDE_DIR "reliable")
set(Yojimbo_SODIUM_INCLUDE_DIR "sodium")
set(Yojimbo_TLSF_INCLUDE_DIR "tlsf")
set(Yojimbo_INCLUDE_DIR "include")
set(Yojimbo_SOURCE_DIR "source")

yojimbo_adlib_interface(serialize INTERFACE
    "${Yojimbo_SERIALIZE_INCLUDE_DIR}" "h"
    )

yojimbo_adlib(reliable STATIC
    "${Yojimbo_RELIABLE_INCLUDE_DIR}" "c"
    "${Yojimbo_RELIABLE_INCLUDE_DIR}" "h"
    )

yojimbo_adlib(tlsf STATIC
    "${Yojimbo_TLSF_INCLUDE_DIR}" "c"
    "${Yojimbo_TLSF_INCLUDE_DIR}" "h"
    )

find_library(sodium NAMES sodium libsodium)

if(NOT sodium) # untested
    if(WIN32)
        yojimbo_adlib(sodium STATIC
            "${Yojimbo_SODIUM_INCLUDE_DIR}" "c"
            "${Yojimbo_SODIUM_INCLUDE_DIR}" "h"
            )
    else()
        yojimbo_adlib(sodium STATIC
            "${Yojimbo_Sodium_INCLUDE_DIR}" "S"
            "" ""
            )
        target_sources(sodium PRIVATE "${Yojimbo_SODIUM_INCLUDE_DIR}/dummy.c")
    endif()
endif()

yojimbo_adlib(netcode STATIC
    "${Yojimbo_NETCODE_INCLUDE_DIR}" "c"
    "${Yojimbo_NETCODE_INCLUDE_DIR}" "h"
    )
target_link_libraries(netcode PRIVATE sodium)

yojimbo_adlib(yojimbo STATIC
    "${Yojimbo_SOURCE_DIR}" "cpp"
    "${Yojimbo_INCLUDE_DIR}" "h"
    )

set_target_properties(yojimbo PROPERTIES OUTPUT_NAME yojimbo)

# yojimbo_allocator.cpp:32 " #include "tlsf/tlsf.h" "
target_include_directories(yojimbo PRIVATE "." PUBLIC Yojimbo_INCLUDE_DIR)

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
