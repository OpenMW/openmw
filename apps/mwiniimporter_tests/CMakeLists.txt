file(GLOB INIIMPORTER_TESTS_SRC_FILES
    main.cpp
)

source_group(apps\\openmw-iniimporter-tests FILES ${INIIMPORTER_TESTS_SRC_FILES})

openmw_add_executable(openmw-iniimporter-tests ${INIIMPORTER_TESTS_SRC_FILES})

target_include_directories(openmw-iniimporter-tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
target_include_directories(openmw-iniimporter-tests SYSTEM PRIVATE ${GMOCK_INCLUDE_DIRS})

target_link_libraries(openmw-iniimporter-tests PRIVATE
    GTest::GTest
    GMock::GMock
)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-iniimporter-tests PRIVATE --coverage)
    target_link_libraries(openmw-iniimporter-tests PRIVATE gcov)
endif()

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
    target_precompile_headers(openmw-iniimporter-tests PRIVATE
        <gtest/gtest.h>
    )
endif()
