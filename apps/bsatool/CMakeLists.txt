set(BSATOOL
    bsatool.cpp
)
source_group(apps\\bsatool FILES ${BSATOOL})

# Main executable
openmw_add_executable(bsatool
    ${BSATOOL}
)

target_link_libraries(bsatool
    Boost::program_options
    components
)

if (WIN32)
    target_sources(bsatool PRIVATE ${CMAKE_SOURCE_DIR}/files/windows/other-apps.manifest)
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(bsatool PRIVATE --coverage)
    target_link_libraries(bsatool gcov)
endif()

if (WIN32)
    install(TARGETS bsatool RUNTIME DESTINATION ".")
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(bsatool REUSE_FROM components)
endif()
