cmake_minimum_required(VERSION 3.11)

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

set(SAVED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

string(REPLACE "-Wsuggest-override" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wundef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
include(FetchContent)
FetchContent_Declare(benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.5.2.zip
    URL_HASH MD5=49395b757a7c4656d70f1328d93efd00
    SOURCE_DIR fetched/benchmark
)
FetchContent_MakeAvailableExcludeFromAll(benchmark)

set(CMAKE_CXX_FLAGS "${SAVED_CMAKE_CXX_FLAGS}")

openmw_add_executable(openmw_detournavigator_navmeshtilescache_benchmark detournavigator/navmeshtilescache.cpp)
target_compile_features(openmw_detournavigator_navmeshtilescache_benchmark PRIVATE cxx_std_17)
target_link_libraries(openmw_detournavigator_navmeshtilescache_benchmark benchmark::benchmark components)

if (UNIX AND NOT APPLE)
    target_link_libraries(openmw_detournavigator_navmeshtilescache_benchmark ${CMAKE_THREAD_LIBS_INIT})
endif()
