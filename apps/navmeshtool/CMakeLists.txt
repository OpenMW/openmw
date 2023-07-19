set(NAVMESHTOOL
    worldspacedata.cpp
    navmesh.cpp
    main.cpp
)
source_group(apps\\navmeshtool FILES ${NAVMESHTOOL})

openmw_add_executable(openmw-navmeshtool ${NAVMESHTOOL})

target_link_libraries(openmw-navmeshtool
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    components
)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-navmeshtool PRIVATE --coverage)
    target_link_libraries(openmw-navmeshtool gcov)
endif()

if (WIN32)
    install(TARGETS openmw-navmeshtool RUNTIME DESTINATION ".")
endif()

if (MSVC)
    target_precompile_headers(openmw-navmeshtool PRIVATE
        <algorithm>
        <memory>
        <string>
        <vector>
    )
endif()
