set(ESSIMPORTER_FILES
    main.cpp
    importer.cpp
    importplayer.cpp
    importnpcc.cpp
    importcrec.cpp
    importcellref.cpp
    importacdt.cpp
    importercontext.cpp
    converter.cpp
    convertacdt.cpp
    convertnpcc.cpp
)

add_executable(openmw-essimporter
    ${ESSIMPORTER_FILES}
)

target_link_libraries(openmw-essimporter
    ${Boost_LIBRARIES}
    components
)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(openmw-essimporter gcov)
endif()
