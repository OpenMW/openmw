cmake_minimum_required(VERSION 2.8)

# This is NOT intended as a stand-alone build system! Instead, you should include this from the main CMakeLists of your project.
# Make sure to link against Ogre and boost::filesystem.

option(SHINY_BUILD_OGRE_PLATFORM "build the Ogre platform" ON)

set(SHINY_LIBRARY "shiny")
set(SHINY_OGREPLATFORM_LIBRARY "shiny.OgrePlatform")

# Sources
set(SOURCE_FILES
    Main/Factory.cpp
    Main/MaterialInstance.cpp
    Main/MaterialInstancePass.cpp
    Main/MaterialInstanceTextureUnit.cpp
    Main/Platform.cpp
    Main/Preprocessor.cpp
    Main/PropertyBase.cpp
    Main/ScriptLoader.cpp
    Main/ShaderInstance.cpp
    Main/ShaderSet.cpp
)

set(OGRE_PLATFORM_SOURCE_FILES
    Platforms/Ogre/OgreGpuProgram.cpp
    Platforms/Ogre/OgreMaterial.cpp
    Platforms/Ogre/OgreMaterialSerializer.cpp
    Platforms/Ogre/OgrePass.cpp
    Platforms/Ogre/OgrePlatform.cpp
    Platforms/Ogre/OgreTextureUnitState.cpp
)

file(GLOB OGRE_PLATFORM_SOURCE_FILES Platforms/Ogre/*.cpp)

add_library(${SHINY_LIBRARY} STATIC ${SOURCE_FILES})

set(SHINY_LIBRARIES ${SHINY_LIBRARY})

if (SHINY_BUILD_OGRE_PLATFORM)
    add_library(${SHINY_OGREPLATFORM_LIBRARY} STATIC ${OGRE_PLATFORM_SOURCE_FILES})
    set(SHINY_LIBRARIES ${SHINY_LIBRARIES} ${SHINY_OGREPLATFORM_LIBRARY})
endif()

set(SHINY_LIBRARY ${SHINY_LIBRARY} PARENT_SCOPE)

if (DEFINED SHINY_BUILD_MATERIAL_EDITOR)
    add_subdirectory(Editor)

    set(SHINY_BUILD_EDITOR_FLAG ${SHINY_BUILD_EDITOR_FLAG} PARENT_SCOPE)
endif()

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(SHINY_LIBRARIES ${SHINY_LIBRARIES} PARENT_SCOPE)
