set(NIFCONV
  nifconv.cpp
)
source_group(apps\\osgtools FILES ${NIFCONV})

# Main executable
add_executable(nifconv
	${NIFCONV}
)

target_link_libraries(nifconv
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(nifconv gcov)
endif()
