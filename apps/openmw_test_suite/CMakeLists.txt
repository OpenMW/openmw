find_package(GTest REQUIRED)

if (GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})

    file(GLOB UNITTEST_SRC_FILES
        ../openmw/mwworld/store.cpp
        ../openmw/mwworld/esmstore.cpp
        mwworld/test_store.cpp

        mwdialogue/test_keywordsearch.cpp

        esm/test_fixed_string.cpp

        misc/test_stringops.cpp
    )

    source_group(apps\\openmw_test_suite FILES openmw_test_suite.cpp ${UNITTEST_SRC_FILES})

    openmw_add_executable(openmw_test_suite openmw_test_suite.cpp ${UNITTEST_SRC_FILES})

    target_link_libraries(openmw_test_suite ${GTEST_BOTH_LIBRARIES} components)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if (UNIX AND NOT APPLE)
        target_link_libraries(openmw_test_suite ${CMAKE_THREAD_LIBS_INIT})
    endif()
endif()


