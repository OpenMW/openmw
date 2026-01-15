set(NAVMESHTOOL_LIB
    worldspacedata.cpp
    navmesh.cpp
)

source_group(apps\\navmeshtool FILES ${NAVMESHTOOL_LIB} main.cpp)

add_library(openmw-navmeshtool-lib STATIC ${NAVMESHTOOL_LIB})

if (ANDROID)
    add_library(openmw-navmeshtool SHARED main.cpp)
else()
    openmw_add_executable(openmw-navmeshtool main.cpp)
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
    target_precompile_headers(openmw-navmeshtool REUSE_FROM components)
endif()
