set(MWINIIMPORT
    main.cpp
    importer.cpp
)

set(MWINIIMPORT_HEADER
    importer.hpp
)

source_group(launcher FILES ${MWINIIMPORT} ${MWINIIMPORT_HEADER})

add_executable(openmw-iniimporter
    ${MWINIIMPORT}
)

target_link_libraries(openmw-iniimporter
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    components
)

if (WIN32)
    target_link_libraries(openmw-iniimporter
    ${Boost_LOCALE_LIBRARY})
endif()

if (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-iniimporter gcov)
endif()
