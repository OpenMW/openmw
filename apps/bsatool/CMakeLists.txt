set(BSATOOL
	bsatool.cpp
	bsatool_cmd.c
	bsatool_cmd.h
)
source_group(apps\\bsatool FILES ${BSATOOL})

# Main executable
add_executable(bsatool
	${BSATOOL}
)

target_link_libraries(bsatool
  ${Boost_LIBRARIES}
  components
)

#if (APPLE)
#    find_library(CARBON_FRAMEWORK Carbon)
#    target_link_libraries(openmw ${CARBON_FRAMEWORK})
#endif (APPLE)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(bsatool gcov)
endif()
