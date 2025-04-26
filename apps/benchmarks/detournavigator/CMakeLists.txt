openmw_add_executable(openmw_detournavigator_navmeshtilescache_benchmark navmeshtilescache.cpp)
target_link_libraries(openmw_detournavigator_navmeshtilescache_benchmark benchmark::benchmark components)

if (UNIX AND NOT APPLE)
    target_link_libraries(openmw_detournavigator_navmeshtilescache_benchmark ${CMAKE_THREAD_LIBS_INIT})
endif()

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(openmw_detournavigator_navmeshtilescache_benchmark PRIVATE <algorithm>)
endif()
