find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

if (GTEST_FOUND AND GMOCK_FOUND)
    include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
    include_directories(SYSTEM ${GMOCK_INCLUDE_DIRS})

    file(GLOB UNITTEST_SRC_FILES
        ../openmw/mwworld/store.cpp
        ../openmw/mwworld/esmstore.cpp
        mwworld/test_store.cpp

        mwdialogue/test_keywordsearch.cpp

        esm/test_fixed_string.cpp

        misc/test_stringops.cpp

        nifloader/testbulletnifloader.cpp

        detournavigator/navigator.cpp
        detournavigator/settingsutils.cpp
        detournavigator/recastmeshbuilder.cpp
        detournavigator/gettilespositions.cpp
        detournavigator/recastmeshobject.cpp
        detournavigator/navmeshtilescache.cpp
        detournavigator/tilecachedrecastmeshmanager.cpp

        settings/parser.cpp
    )

    source_group(apps\\openmw_test_suite FILES openmw_test_suite.cpp ${UNITTEST_SRC_FILES})

    openmw_add_executable(openmw_test_suite openmw_test_suite.cpp ${UNITTEST_SRC_FILES})

    target_link_libraries(openmw_test_suite ${GMOCK_LIBRARIES} components)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if (UNIX AND NOT APPLE)
        target_link_libraries(openmw_test_suite ${CMAKE_THREAD_LIBS_INIT})
    endif()

    if (BUILD_WITH_CODE_COVERAGE)
        add_definitions(--coverage)
        target_link_libraries(openmw_test_suite gcov)
    endif()

    if (MSVC)
        if (CMAKE_CL_64)
            # Debug version of openmw_unit_tests needs increased number of sections beyond 2^16
            # just like openmw and openmw-cs
            set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
        endif (CMAKE_CL_64)
    endif (MSVC)

endif()
