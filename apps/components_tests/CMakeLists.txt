include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
include_directories(SYSTEM ${GMOCK_INCLUDE_DIRS})

file(GLOB UNITTEST_SRC_FILES
    main.cpp

    esm/test_fixed_string.cpp
    esm/variant.cpp
    esm/testrefid.cpp

    lua/test_lua.cpp
    lua/test_scriptscontainer.cpp
    lua/test_utilpackage.cpp
    lua/test_serialization.cpp
    lua/test_configuration.cpp
    lua/test_l10n.cpp
    lua/test_storage.cpp
    lua/test_async.cpp
    lua/test_inputactions.cpp
    lua/test_yaml.cpp

    lua/test_ui_content.cpp

    misc/test_stringops.cpp
    misc/test_endianness.cpp
    misc/test_resourcehelpers.cpp
    misc/progressreporter.cpp
    misc/compression.cpp

    nifloader/testbulletnifloader.cpp

    detournavigator/navigator.cpp
    detournavigator/settingsutils.cpp
    detournavigator/recastmeshbuilder.cpp
    detournavigator/gettilespositions.cpp
    detournavigator/recastmeshobject.cpp
    detournavigator/navmeshtilescache.cpp
    detournavigator/tilecachedrecastmeshmanager.cpp
    detournavigator/navmeshdb.cpp
    detournavigator/serialization.cpp
    detournavigator/asyncnavmeshupdater.cpp

    serialization/binaryreader.cpp
    serialization/binarywriter.cpp
    serialization/sizeaccumulator.cpp
    serialization/integration.cpp

    settings/parser.cpp
    settings/shadermanager.cpp
    settings/testvalues.cpp

    shader/parsedefines.cpp
    shader/parsefors.cpp
    shader/parselinks.cpp
    shader/shadermanager.cpp

    sqlite3/db.cpp
    sqlite3/request.cpp
    sqlite3/statement.cpp
    sqlite3/transaction.cpp

    esmloader/load.cpp
    esmloader/esmdata.cpp
    esmloader/record.cpp

    files/hash.cpp
    files/conversion_tests.cpp

    toutf8/toutf8.cpp

    esm4/includes.cpp

    fx/lexer.cpp
    fx/technique.cpp

    esm3/readerscache.cpp
    esm3/testsaveload.cpp
    esm3/testesmwriter.cpp
    esm3/testinfoorder.cpp

    nifosg/testnifloader.cpp

    esmterrain/testgridsampling.cpp

    resource/testobjectcache.cpp

    vfs/testpathutil.cpp

    sceneutil/osgacontroller.cpp
)

source_group(apps\\components-tests FILES ${UNITTEST_SRC_FILES})

openmw_add_executable(components-tests ${UNITTEST_SRC_FILES})

target_link_libraries(components-tests
    GTest::GTest
    GMock::GMock
    components
)

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    target_link_libraries(components-tests ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(components-tests PRIVATE --coverage)
    target_link_libraries(components-tests gcov)
endif()

file(DOWNLOAD
    https://gitlab.com/OpenMW/example-suite/-/raw/8966dab24692555eec720c854fb0f73d108070cd/data/template.omwgame
    ${CMAKE_CURRENT_BINARY_DIR}/data/template.omwgame
    EXPECTED_HASH SHA512=6e38642bcf013c5f496a9cb0bf3ec7c9553b6e86b836e7844824c5a05f556c9391167214469b6318401684b702d7569896bf743c85aee4198612b3315ba778d6
)

target_compile_definitions(components-tests
    PRIVATE OPENMW_DATA_DIR=u8"${CMAKE_CURRENT_BINARY_DIR}/data"
            OPENMW_PROJECT_SOURCE_DIR=u8"${PROJECT_SOURCE_DIR}")

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(components-tests PRIVATE
        <boost/program_options/options_description.hpp>

        <gtest/gtest.h>

        <sol/sol.hpp>

        <algorithm>
        <filesystem>
        <fstream>
        <functional>
        <memory>
        <string>
        <vector>
    )
endif()
