# TODO: This should not be needed, check how it was done in FindGTEST
set(GMOCK_ROOT "/usr/include")
set(GMOCK_BUILD "/usr/lib")

find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

if (GTEST_FOUND AND GMOCK_FOUND)

    include_directories(${GTEST_INCLUDE_DIRS})
    include_directories(${GMOCK_INCLUDE_DIRS})

    file(GLOB UNITTEST_SRC_FILES
        components/misc/test_*.cpp
    )

    source_group(apps\\openmw_test_suite FILES openmw_test_suite.cpp ${UNITTEST_SRC_FILES})

    add_executable(openmw_test_suite openmw_test_suite.cpp ${UNITTEST_SRC_FILES})

    target_link_libraries(openmw_test_suite ${GMOCK_BOTH_LIBRARIES} ${GTEST_BOTH_LIBRARIES} components)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if (UNIX AND NOT APPLE)
        target_link_libraries(openmw_test_suite ${CMAKE_THREAD_LIBS_INIT})
    endif()
endif()


