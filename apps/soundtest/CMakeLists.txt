project(SoundTest)

# local files

# Main executable
add_executable(sound_test main.cpp ${OENGINE_SOUND})

include_directories(${SOUND_INPUT_INCLUDES})
add_definitions(${SOUND_DEFINE})

target_link_libraries(sound_test
  ${OPENAL_LIBRARY}
  ${SOUND_INPUT_LIBRARY}
)
