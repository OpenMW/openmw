set(MWINIIMPORT
    main.cpp
    importer.cpp
)

set(MWINIIMPORT_HEADER
    importer.hpp
)

source_group(launcher FILES ${MWINIIMPORT} ${MWINIIMPORT_HEADER})

openmw_add_executable(openmw-iniimporter
    ${MWINIIMPORT}
)

target_link_libraries(openmw-iniimporter
    Boost::program_options
    components
)

if (WIN32)
    target_link_libraries(openmw-iniimporter
        Boost::locale)
    INSTALL(TARGETS openmw-iniimporter RUNTIME DESTINATION ".")
endif(WIN32)

if (MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-iniimporter PRIVATE --coverage)
    target_link_libraries(openmw-iniimporter gcov)
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw-iniimporter PRIVATE
        <string>
        <vector>
    )
endif()
