project(SoundTest)

# local files

# Main executable
add_executable(sound_test main.cpp ${OENGINE_SOUND})

target_link_libraries(sound_test
  ${OPENAL_LIBRARY}
  ${AUDIERE_LIBRARY}
)
