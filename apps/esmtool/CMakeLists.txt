set(ESMTOOL
  esmtool.cpp
  labels.hpp
  labels.cpp
  record.hpp
  record.cpp
)
source_group(apps\\esmtool FILES ${ESMTOOL})

# Main executable
openmw_add_executable(esmtool
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
