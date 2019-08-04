cmake_minimum_required(VERSION 3.0)

project(RecastNavigation)

# lib versions
SET(SOVERSION 1)
SET(VERSION 1.0.0)

option(RECASTNAVIGATION_STATIC "Build static libraries" ON)

add_subdirectory(DebugUtils)
add_subdirectory(Detour)
add_subdirectory(DetourCrowd)
add_subdirectory(DetourTileCache)
add_subdirectory(Recast)
