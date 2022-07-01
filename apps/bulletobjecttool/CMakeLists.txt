set(BULLETMESHTOOL
    main.cpp
)
source_group(apps\\bulletobjecttool FILES ${BULLETMESHTOOL})

openmw_add_executable(openmw-bulletobjecttool ${BULLETMESHTOOL})

target_link_libraries(openmw-bulletobjecttool
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    components
)

if (BUILD_WITH_CODE_COVERAGE)
    add_definitions(--coverage)
    target_link_libraries(openmw-bulletobjecttool gcov)
endif()

if (WIN32)
    install(TARGETS openmw-bulletobjecttool RUNTIME DESTINATION ".")
endif()

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
    target_precompile_headers(openmw-bulletobjecttool PRIVATE
        <string>
        <vector>
    )
endif()
