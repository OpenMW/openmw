# AI Dialogue System Component
# Provides AI-driven dialogue generation using OpenRouter API

set(COMPONENT_FILES
    json.hpp
    json.cpp
    openrouterclient.hpp
    openrouterclient.cpp
    contextbuilder.hpp
    contextbuilder.cpp
    dialoguegenerator.hpp
    dialoguegenerator.cpp
    commandinterpreter.hpp
    commandinterpreter.cpp
)

add_library(aidialogue STATIC ${COMPONENT_FILES})

target_include_directories(aidialogue PUBLIC
    ${CMAKE_SOURCE_DIR}/components
    ${CMAKE_SOURCE_DIR}/apps
)

# Link against required libraries
target_link_libraries(aidialogue
    PUBLIC
        components
    PRIVATE
        CURL::libcurl
)

# Require C++17 for std::optional and other features
target_compile_features(aidialogue PUBLIC cxx_std_17)

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(aidialogue PRIVATE --coverage)
    target_link_libraries(aidialogue PRIVATE gcov)
endif()
