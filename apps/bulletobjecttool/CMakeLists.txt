set(BULLETMESHTOOL
    main.cpp
)
source_group(apps\\bulletobjecttool FILES ${BULLETMESHTOOL})

openmw_add_executable(openmw-bulletobjecttool ${BULLETMESHTOOL})

target_link_libraries(openmw-bulletobjecttool
    Boost::program_options
    components
)

if (WIN32)
    install(TARGETS openmw-bulletobjecttool RUNTIME DESTINATION ".")
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw-bulletobjecttool PRIVATE
        <string>
        <vector>
    )
endif()
