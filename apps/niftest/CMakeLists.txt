set(NIFTEST
    niftest.cpp
)
source_group(components\\nif\\tests FILES ${NIFTEST})

# Main executable
openmw_add_executable(niftest
    ${NIFTEST}
)

target_link_libraries(niftest
    components
)

if (WIN32)
    install(TARGETS niftest RUNTIME DESTINATION ".")
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(niftest PRIVATE <filesystem>)
endif()
