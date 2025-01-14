set(ESMTOOL
    esmtool.cpp
    labels.hpp
    labels.cpp
    record.hpp
    record.cpp
    arguments.hpp
    tes4.hpp
    tes4.cpp
)
source_group(apps\\esmtool FILES ${ESMTOOL})

# Main executable
openmw_add_executable(esmtool
    ${ESMTOOL}
)

target_link_libraries(esmtool
    Boost::program_options
    components
)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(esmtool PRIVATE --coverage)
    target_link_libraries(esmtool gcov)
endif()

if (WIN32)
    install(TARGETS esmtool RUNTIME DESTINATION ".")
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(esmtool PRIVATE
        <fstream>
        <string>
        <vector>
    )
endif()
