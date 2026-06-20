add_subdirectory(yojimbo EXCLUDE_FROM_ALL)

function(openmw_disable_warnings_as_errors_for_third_party_target target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    get_target_property(target_type ${target_name} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    # libyojimbo is vendored middleware, not OpenMW code. Keep OpenMW strict,
    # but don't make upstream warning churn an update tax for this target.
    if(MSVC OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target_name} PRIVATE /WX-)
    else()
        target_compile_options(${target_name} PRIVATE -Wno-error)
    endif()
endfunction()

function(openmw_make_interface_includes_system target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    get_target_property(interface_includes ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT interface_includes)
        return()
    endif()

    set_target_properties(${target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
    target_include_directories(${target_name} SYSTEM INTERFACE ${interface_includes})
endfunction()

function(openmw_clear_third_party_interface_sources target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    # yojimbo's CMake advertises relative header paths through PUBLIC
    # target_sources. Consumers don't need those headers as sources, and recent
    # CMake versions reject the relative INTERFACE_SOURCES entries during
    # generation. Quarantine that upstream CMake wart here.
    set_target_properties(${target_name} PROPERTIES INTERFACE_SOURCES "")
endfunction()

foreach(target_name IN ITEMS
    yojimbo
    netcode
    reliable
    tlsf
    sodium
    serialize
)
    openmw_disable_warnings_as_errors_for_third_party_target(${target_name})
    openmw_make_interface_includes_system(${target_name})
    openmw_clear_third_party_interface_sources(${target_name})
endforeach()
