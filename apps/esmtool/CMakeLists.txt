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
  ${Boost_LIBRARIES}
  components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(esmtool gcov)
endif()
