set(BSATOOL
	bsatool.cpp
)
source_group(apps\\bsatool FILES ${BSATOOL})

# Main executable
add_executable(bsatool
	${BSATOOL}
)

target_link_libraries(bsatool
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(bsatool gcov)
endif()

if (MSVC)
	if (CMAKE_VERSION VERSION_GREATER 3.8)
		set_target_properties(bsatool PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)")
	endif (CMAKE_VERSION VERSION_GREATER 3.8)
endif (MSVC)