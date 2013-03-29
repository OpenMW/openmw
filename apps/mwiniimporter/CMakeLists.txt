set(MWINIIMPORT
    main.cpp
    importer.cpp
)

set(MWINIIMPORT_HEADER
    importer.hpp
)

source_group(launcher FILES ${MWINIIMPORT} ${MWINIIMPORT_HEADER})

add_executable(mwiniimport
    ${MWINIIMPORT}
)

target_link_libraries(mwiniimport
    ${Boost_LIBRARIES}
    components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(mwiniimport gcov)
endif()

if(DPKG_PROGRAM)
    INSTALL(TARGETS mwiniimport RUNTIME DESTINATION games COMPONENT mwiniimport)
endif()

