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
        Boost::filesystem
        components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(bsatool gcov)
endif()
