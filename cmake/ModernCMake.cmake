cmake_minimum_required(VERSION 3.4)

function(compile_conan_dependencies conanFile)
    find_program(CONAN_CMD conan
        DOC "Conan executable location"
        PATHS /usr/local/bin
        )
    
    message("Running Conan install")
    if(CMAKE_BUILD_TYPE STREQUAL "")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    include(conan)
    conan_cmake_run(CONANFILE ${conanFile}
                    BASIC_SETUP CMAKE_TARGETS
                    BUILD missing
                    CONAN_COMMAND ${CONAN_CMD})    
endfunction(compile_conan_dependencies)

function(__copy_conan_target dst src)
    add_library(${dst} INTERFACE IMPORTED)
    foreach(name INTERFACE_LINK_LIBRARIES INTERFACE_INCLUDE_DIRECTORIES INTERFACE_COMPILE_DEFINITIONS INTERFACE_COMPILE_OPTIONS)
        get_property(value TARGET ${src} PROPERTY ${name} )
        set_property(TARGET ${dst} PROPERTY ${name} ${value})
    endforeach()
endfunction()

macro(expose_package_conan tgt)
    if((TARGET CONAN_PKG::${tgt}) AND (NOT TARGET ${tgt}))
        message(STATUS "Exposing conan target CONAN_PKG::${tgt} as ${tgt}")
        __copy_conan_target(${tgt} CONAN_PKG::${tgt})
        list(APPEND ALREADY_FOUND_TARGETS ${tgt})
        foreach(h ${ARGN})
            message(STATUS "Exposing conan target CONAN_PKG::${tgt} also as ${h}")
            __copy_conan_target(${h} CONAN_PKG::${tgt})
            list(APPEND ALREADY_FOUND_TARGETS ${h})
        endforeach()
    else()
        if(NOT TARGET CONAN_PKG::${tgt})
            message(FATAL_ERROR "Could not expose ${tgt} from conan")
        endif()
    endif()
endmacro(expose_package_conan)

macro(package_in_project tgt)
    list(APPEND ALREADY_FOUND_TARGETS ${tgt})    
endmacro()

macro(find_package)
    if(NOT "${ARGV0}" IN_LIST ALREADY_FOUND_TARGETS)
        message(STATUS "Using find_package: ${ARGV0}")
        _find_package(${ARGV})
    else()
        message(STATUS "Target already provided: ${ARGV0}, so find_package call skipped")
    endif()
endmacro()

macro(target_sources_public_headers tgt)
    foreach(h ${ARGN})
        target_sources(${tgt} PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${h}>  
            $<INSTALL_INTERFACE:include/${h}>)
    endforeach()
endmacro()

macro(target_sources_interface_headers tgt)
    foreach(h ${ARGN})
        target_sources(${tgt} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${h}>  
            $<INSTALL_INTERFACE:include/${h}>)
    endforeach()
endmacro()

macro(install_target tgt)
    install(TARGETS ${tgt}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    )
endmacro()

macro(export_library tgt headersdir namespace version)
    target_include_directories(${tgt} PUBLIC
        $<BUILD_INTERFACE:${${tgt}_BINARY_DIR}>
        $<BUILD_INTERFACE:${${tgt}_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    
    install(
      DIRECTORY
        ${headersdir}
      DESTINATION
        include
      COMPONENT
        Devel
    )
    
    install(TARGETS ${tgt} EXPORT ${tgt}Targets 
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
    ) 
    
    install(EXPORT ${tgt}Targets
      FILE ${tgt}Targets.cmake
      NAMESPACE ${namespace}::
      DESTINATION lib/cmake/${namespace}
      )
    
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(${tgt}ConfigVersion.cmake
        VERSION ${version}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES 
                ${tgt}Config.cmake 
                ${CMAKE_CURRENT_BINARY_DIR}/${tgt}ConfigVersion.cmake
        DESTINATION lib/cmake/${namespace})

    add_library(${namespace}::${tgt} ALIAS ${tgt})
    
endmacro()
