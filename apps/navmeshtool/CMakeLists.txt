set(NAVMESHTOOL
    worldspacedata.cpp
    navmesh.cpp
    main.cpp
)
source_group(apps\\navmeshtool FILES ${NAVMESHTOOL})

add_library(openmw-navmeshtool-lib STATIC
    ${NAVMESHTOOL}
)

if (ANDROID)
    add_library(openmw-navmeshtool SHARED
        main.cpp
    )
else()
    openmw_add_executable(openmw-navmeshtool ${NAVMESHTOOL})
endif()

target_link_libraries(openmw-navmeshtool openmw-navmeshtool-lib)

target_link_libraries(openmw-navmeshtool-lib
    Boost::program_options
    components
)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-navmeshtool PRIVATE --coverage)
    target_link_libraries(openmw-navmeshtool gcov)
endif()

if (WIN32)
    install(TARGETS openmw-navmeshtool RUNTIME DESTINATION ".")
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw-navmeshtool PRIVATE
        <algorithm>
        <memory>
        <string>
        <vector>
    )
endif()
