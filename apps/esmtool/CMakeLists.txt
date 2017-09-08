set(ESMTOOL
  esmtool.cpp
  labels.hpp
  labels.cpp
  record.hpp
  record.cpp
)
source_group(apps\\esmtool FILES ${ESMTOOL})

# Main executable
add_executable(esmtool
  ${ESMTOOL}
)

target_link_libraries(esmtool
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(esmtool gcov)
endif()

if (MSVC)
	if (CMAKE_VERSION VERSION_GREATER 3.8)
		set_target_properties(esmtool PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)")
	endif (CMAKE_VERSION VERSION_GREATER 3.8)
endif (MSVC)