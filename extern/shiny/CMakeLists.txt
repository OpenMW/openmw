cmake_minimum_required(VERSION 2.8)

# This is NOT intended as a stand-alone build system! Instead, you should include this from the main CMakeLists of your project.
# Make sure to link against Ogre and boost::filesystem.

option(SHINY_BUILD_OGRE_PLATFORM "build the Ogre platform" ON)

set(SHINY_LIBRARY "shiny")
set(SHINY_OGREPLATFORM_LIBRARY "shiny.OgrePlatform")

# Sources
file(GLOB SOURCE_FILES Main/*.cpp )

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

# In Debug mode, write the shader sources to the current directory
if (DEFINED CMAKE_BUILD_TYPE)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DSHINY_WRITE_SHADER_DEBUG)
    endif()
endif()

if (DEFINED SHINY_USE_WAVE_SYSTEM_INSTALL)
    # use system install
else()
    list(APPEND SOURCE_FILES
        Preprocessor/aq.cpp
        Preprocessor/cpp_re.cpp
        Preprocessor/instantiate_cpp_literalgrs.cpp
        Preprocessor/instantiate_cpp_exprgrammar.cpp
        Preprocessor/instantiate_cpp_grammar.cpp
        Preprocessor/instantiate_defined_grammar.cpp
        Preprocessor/instantiate_predef_macros.cpp
        Preprocessor/instantiate_re2c_lexer.cpp
        Preprocessor/instantiate_re2c_lexer_str.cpp
        Preprocessor/token_ids.cpp
    )

    # Don't use thread-safe boost::wave. Results in a huge speed-up for the preprocessor.
    add_definitions(-DBOOST_WAVE_SUPPORT_THREADING=0)
endif()

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

if (SHINY_BUILD_OGRE_PLATFORM)
    add_library(${SHINY_OGREPLATFORM_LIBRARY} STATIC ${OGRE_PLATFORM_SOURCE_FILES})
endif()


link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SHINY_LIBRARY ${SHINY_LIBRARY} PARENT_SCOPE)
set(SHINY_OGREPLATFORM_LIBRARY ${SHINY_OGREPLATFORM_LIBRARY} PARENT_SCOPE)
