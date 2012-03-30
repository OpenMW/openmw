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

