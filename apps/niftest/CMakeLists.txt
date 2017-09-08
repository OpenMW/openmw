set(NIFTEST
    niftest.cpp
)
source_group(components\\nif\\tests FILES ${NIFTEST})

# Main executable
add_executable(niftest
    ${NIFTEST}
)

target_link_libraries(niftest
  ${Boost_FILESYSTEM_LIBRARY}
  components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(niftest gcov)
endif()

if (MSVC)
	if (CMAKE_VERSION VERSION_GREATER 3.8)
		set_target_properties(niftest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)")
	endif (CMAKE_VERSION VERSION_GREATER 3.8)
endif (MSVC)