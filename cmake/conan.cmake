# The MIT License (MIT)

# Copyright (c) 2018 JFrog

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.



# This file comes from: https://github.com/conan-io/cmake-conan. Please refer
# to this repository for issues and documentation.

# Its purpose is to wrap and launch Conan C/C++ Package Manager when cmake is called.
# It will take CMake current settings (os, compiler, compiler version, architecture)
# and translate them to conan settings for installing and retrieving dependencies.

# It is intended to facilitate developers building projects that have conan dependencies,
# but it is only necessary on the end-user side. It is not necessary to create conan
# packages, in fact it shouldn't be use for that. Check the project documentation.


include(CMakeParseArguments)

function(_get_msvc_ide_version result)
    set(${result} "" PARENT_SCOPE)
    if(NOT MSVC_VERSION VERSION_LESS 1400 AND MSVC_VERSION VERSION_LESS 1500)
        set(${result} 8 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1500 AND MSVC_VERSION VERSION_LESS 1600)
        set(${result} 9 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1600 AND MSVC_VERSION VERSION_LESS 1700)
        set(${result} 10 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1700 AND MSVC_VERSION VERSION_LESS 1800)
        set(${result} 11 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1800 AND MSVC_VERSION VERSION_LESS 1900)
        set(${result} 12 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1900 AND MSVC_VERSION VERSION_LESS 1910)
        set(${result} 14 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1910 AND MSVC_VERSION VERSION_LESS 1920)
        set(${result} 15 PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Conan: Unknown MSVC compiler version [${MSVC_VERSION}]")
    endif()
endfunction()

function(conan_cmake_settings result)
    #message(STATUS "COMPILER " ${CMAKE_CXX_COMPILER})
    #message(STATUS "COMPILER " ${CMAKE_CXX_COMPILER_ID})
    #message(STATUS "VERSION " ${CMAKE_CXX_COMPILER_VERSION})
    #message(STATUS "FLAGS " ${CMAKE_LANG_FLAGS})
    #message(STATUS "LIB ARCH " ${CMAKE_CXX_LIBRARY_ARCHITECTURE})
    #message(STATUS "BUILD TYPE " ${CMAKE_BUILD_TYPE})
    #message(STATUS "GENERATOR " ${CMAKE_GENERATOR})
    #message(STATUS "GENERATOR WIN64 " ${CMAKE_CL_64})

    message(STATUS "Conan ** WARNING** : This detection of settings from cmake is experimental and incomplete. "
                    "Please check 'conan.cmake' and contribute")

    parse_arguments(${ARGV})
    set(arch ${ARGUMENTS_ARCH})

    if(CONAN_CMAKE_MULTI)
        set(_SETTINGS -g cmake_multi)
    else()
        set(_SETTINGS -g cmake)
    endif()
    if(ARGUMENTS_BUILD_TYPE)
        set(_SETTINGS ${_SETTINGS} -s build_type=${ARGUMENTS_BUILD_TYPE})
    elseif(CMAKE_BUILD_TYPE)
        set(_SETTINGS ${_SETTINGS} -s build_type=${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Please specify in command line CMAKE_BUILD_TYPE (-DCMAKE_BUILD_TYPE=Release)")
    endif()

    if(CMAKE_CROSSCOMPILING)
        set(_SETTINGS ${_SETTINGS} -s arch=${CMAKE_SYSTEM_PROCESSOR} -s arch_target=${CMAKE_SYSTEM_PROCESSOR} -e CONAN_CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
    endif()
    #handle -s os setting
    if(CMAKE_SYSTEM_NAME)
        #use default conan os setting if CMAKE_SYSTEM_NAME is not defined
        set(CONAN_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            set(CONAN_SYSTEM_NAME Macos)
        endif()
        set(CONAN_SUPPORTED_PLATFORMS Windows Linux Macos Android iOS FreeBSD)
        list (FIND CONAN_SUPPORTED_PLATFORMS "${CONAN_SYSTEM_NAME}" _index)
        if (${_index} GREATER -1)
            #check if the cmake system is a conan supported one
            set(_SETTINGS ${_SETTINGS} -s os=${CONAN_SYSTEM_NAME})
        else()
            message(FATAL_ERROR "cmake system ${CONAN_SYSTEM_NAME} is not supported by conan. Use one of ${CONAN_SUPPORTED_PLATFORMS}")
        endif()
    endif()

    get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if (";${_languages};" MATCHES ";CXX;")
        set(LANGUAGE CXX)
        set(USING_CXX 1)
    elseif (";${_languages};" MATCHES ";C;")
        set(LANGUAGE C)
        set(USING_CXX 0)
    else ()
        message(FATAL_ERROR "Conan: Neither C or C++ was detected as a language for the project. Unabled to detect compiler version.")
    endif()

    if(arch)
        set(_SETTINGS ${_SETTINGS} -s arch=${arch})
    endif()

    if (${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL GNU)
        # using GCC
        # TODO: Handle other params
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_${LANGUAGE}_COMPILER_VERSION})
        list(GET VERSION_LIST 0 MAJOR)
        list(GET VERSION_LIST 1 MINOR)
        set(COMPILER_VERSION ${MAJOR}.${MINOR})
        if(${MAJOR} GREATER 4)
            set(COMPILER_VERSION ${MAJOR})
        endif()
        set(_SETTINGS ${_SETTINGS} -s compiler=gcc -s compiler.version=${COMPILER_VERSION})
        if (USING_CXX)
          conan_cmake_detect_gnu_libcxx(_LIBCXX)
            set(_SETTINGS ${_SETTINGS} -s compiler.libcxx=${_LIBCXX})
        endif ()
    elseif (${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL AppleClang)
        # using AppleClang
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_${LANGUAGE}_COMPILER_VERSION})
        list(GET VERSION_LIST 0 MAJOR)
        list(GET VERSION_LIST 1 MINOR)
        set(_SETTINGS ${_SETTINGS} -s compiler=apple-clang -s compiler.version=${MAJOR}.${MINOR})
        if (USING_CXX)
            set(_SETTINGS ${_SETTINGS} -s compiler.libcxx=libc++)
        endif ()
    elseif (${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL Clang)
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_${LANGUAGE}_COMPILER_VERSION})
        list(GET VERSION_LIST 0 MAJOR)
        list(GET VERSION_LIST 1 MINOR)
        if(APPLE)
            cmake_policy(GET CMP0025 APPLE_CLANG_POLICY_ENABLED)
            if(NOT APPLE_CLANG_POLICY_ENABLED)
                message(STATUS "Conan: APPLE and Clang detected. Assuming apple-clang compiler. Set CMP0025 to avoid it")
                set(_SETTINGS ${_SETTINGS} -s compiler=apple-clang -s compiler.version=${MAJOR}.${MINOR})
            else()
                set(_SETTINGS ${_SETTINGS} -s compiler=clang -s compiler.version=${MAJOR}.${MINOR})
            endif()
            if (USING_CXX)
                set(_SETTINGS ${_SETTINGS} -s compiler.libcxx=libc++)
            endif ()
        else()
            set(_SETTINGS ${_SETTINGS} -s compiler=clang -s compiler.version=${MAJOR}.${MINOR})
            if (USING_CXX)
                conan_cmake_detect_gnu_libcxx(_LIBCXX)
                set(_SETTINGS ${_SETTINGS} -s compiler.libcxx=${_LIBCXX})
            endif ()
        endif()
    elseif(${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL MSVC)
        set(_VISUAL "Visual Studio")
        _get_msvc_ide_version(_VISUAL_VERSION)
        if("${_VISUAL_VERSION}" STREQUAL "")
            message(FATAL_ERROR "Conan: Visual Studio not recognized")
        else()
            set(_SETTINGS ${_SETTINGS} -s compiler=${_VISUAL} -s compiler.version=${_VISUAL_VERSION})
        endif()

        if(NOT arch)
            if (MSVC_${LANGUAGE}_ARCHITECTURE_ID MATCHES "64")
                set(_SETTINGS ${_SETTINGS} -s arch=x86_64)
            elseif (MSVC_${LANGUAGE}_ARCHITECTURE_ID MATCHES "^ARM")
                message(STATUS "Conan: Using default ARM architecture from MSVC")
                set(_SETTINGS ${_SETTINGS} -s arch=armv6)
            elseif (MSVC_${LANGUAGE}_ARCHITECTURE_ID MATCHES "86")
                set(_SETTINGS ${_SETTINGS} -s arch=x86)
            else ()
                message(FATAL_ERROR "Conan: Unknown MSVC architecture [${MSVC_${LANGUAGE}_ARCHITECTURE_ID}]")
            endif()
        endif()

        conan_cmake_detect_vs_runtime(_vs_runtime)
        message(STATUS "Detected VS runtime: ${_vs_runtime}")
        set(_SETTINGS ${_SETTINGS} -s compiler.runtime=${_vs_runtime})
        
        if (CMAKE_GENERATOR_TOOLSET)
            set(_SETTINGS ${_SETTINGS} -s compiler.toolset=${CMAKE_GENERATOR_TOOLSET})
        elseif(CMAKE_VS_PLATFORM_TOOLSET AND (CMAKE_GENERATOR STREQUAL "Ninja"))
            set(_SETTINGS ${_SETTINGS} -s compiler.toolset=${CMAKE_VS_PLATFORM_TOOLSET})
        endif()
    else()
        message(FATAL_ERROR "Conan: compiler setup not recognized")
    endif()

    set(${result} ${_SETTINGS} PARENT_SCOPE)
endfunction()


function(conan_cmake_detect_gnu_libcxx result)
    # Allow -D_GLIBCXX_USE_CXX11_ABI=ON/OFF as argument to cmake
    if(DEFINED _GLIBCXX_USE_CXX11_ABI)
        if(_GLIBCXX_USE_CXX11_ABI)
            set(${result} libstdc++11 PARENT_SCOPE)
            return()
        else()
            set(${result} libstdc++ PARENT_SCOPE)
            return()
        endif()
    endif()

    # Check if there's any add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    get_directory_property(defines DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)
    foreach(define ${defines})
        if(define STREQUAL "_GLIBCXX_USE_CXX11_ABI=0")
            set(${result} libstdc++ PARENT_SCOPE)
            return()
        endif()
    endforeach()

    # Use C++11 stdlib as default if gcc is 5.1+
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.1")
      set(${result} libstdc++ PARENT_SCOPE)
    else()
      set(${result} libstdc++11 PARENT_SCOPE)
    endif()
endfunction()


function(conan_cmake_detect_vs_runtime result)
    string(TOUPPER ${CMAKE_BUILD_TYPE} build_type)
    set(variables CMAKE_CXX_FLAGS_${build_type} CMAKE_C_FLAGS_${build_type} CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    foreach(variable ${variables})
        string(REPLACE " " ";" flags ${${variable}})
        foreach (flag ${flags})
            if(${flag} STREQUAL "/MD" OR ${flag} STREQUAL "/MDd" OR ${flag} STREQUAL "/MT" OR ${flag} STREQUAL "/MTd")
                string(SUBSTRING ${flag} 1 -1 runtime)
                set(${result} ${runtime} PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endforeach()
    if(${build_type} STREQUAL "DEBUG")
        set(${result} "MDd" PARENT_SCOPE)
    else()
        set(${result} "MD" PARENT_SCOPE)
    endif()
endfunction()


macro(parse_arguments)
  set(options BASIC_SETUP CMAKE_TARGETS UPDATE KEEP_RPATHS NO_OUTPUT_DIRS)
  set(oneValueArgs CONANFILE DEBUG_PROFILE RELEASE_PROFILE PROFILE ARCH BUILD_TYPE)
  set(multiValueArgs REQUIRES OPTIONS IMPORTS BUILD CONAN_COMMAND)
  cmake_parse_arguments(ARGUMENTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
endmacro()

function(conan_cmake_install)
    # Calls "conan install"
    # Argument BUILD is equivalant to --build={missing, PkgName,...} or
    # --build when argument is 'BUILD all' (which builds all packages from source)
    # Argument CONAN_COMMAND, to specify the conan path, e.g. in case of running from source
    # cmake does not identify conan as command, even if it is +x and it is in the path
    parse_arguments(${ARGV})

    set(CONAN_BUILD_POLICY "")
    foreach(ARG ${ARGUMENTS_BUILD})
        if(${ARG} STREQUAL "all")
            set(CONAN_BUILD_POLICY ${CONAN_BUILD_POLICY} --build)
            break()
        else()
            set(CONAN_BUILD_POLICY ${CONAN_BUILD_POLICY} --build=${ARG})
        endif()
    endforeach()
    if(ARGUMENTS_CONAN_COMMAND)
       set(conan_command ${ARGUMENTS_CONAN_COMMAND})
    else()
      set(conan_command conan)
    endif()
    set(CONAN_OPTIONS "")
    if(ARGUMENTS_CONANFILE)
      set(CONANFILE ${CMAKE_CURRENT_SOURCE_DIR}/${ARGUMENTS_CONANFILE})
      # A conan file has been specified - apply specified options as well if provided
      foreach(ARG ${ARGUMENTS_OPTIONS})
          set(CONAN_OPTIONS ${CONAN_OPTIONS} -o ${ARG})
      endforeach()
    else()
      set(CONANFILE ".")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ARGUMENTS_DEBUG_PROFILE)
      set(settings -pr ${ARGUMENTS_DEBUG_PROFILE})
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ARGUMENTS_RELEASE_PROFILE)
      set(settings -pr ${ARGUMENTS_RELEASE_PROFILE})
    endif()
    if(ARGUMENTS_PROFILE)
      set(settings -pr ${ARGUMENTS_PROFILE})
    endif()
    if(ARGUMENTS_UPDATE)
      set(CONAN_INSTALL_UPDATE --update)
    endif()
    set(conan_args install ${CONANFILE} ${settings} ${CONAN_BUILD_POLICY} ${CONAN_INSTALL_UPDATE} ${CONAN_OPTIONS})

    string (REPLACE ";" " " _conan_args "${conan_args}")
    message(STATUS "Conan executing: ${conan_command} ${_conan_args}")

    set(ENV{PATH}  "$ENV{PATH}:/usr/local/bin/")
    execute_process(COMMAND ${conan_command} ${conan_args}
                     RESULT_VARIABLE return_code
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     
                     ERROR_VARIABLE CMD_ERROR
                     OUTPUT_VARIABLE CMD_OUTPUT)
                     

    if(NOT "${return_code}" STREQUAL "0")
        message( STATUS "Conan generation error output:" ${CMD_ERROR})
        message( STATUS "Conan generation normal output:" ${CMD_OUTPUT})
      message(FATAL_ERROR "Conan install failed='${return_code}'")
    endif()

endfunction()


function(conan_cmake_setup_conanfile)
  parse_arguments(${ARGV})
  if(ARGUMENTS_CONANFILE)
    # configure_file will make sure cmake re-runs when conanfile is updated
    configure_file(${ARGUMENTS_CONANFILE} ${ARGUMENTS_CONANFILE}.junk)
    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${ARGUMENTS_CONANFILE}.junk)
  else()
    conan_cmake_generate_conanfile(${ARGV})
  endif()
endfunction()

function(conan_cmake_generate_conanfile)
  # Generate, writing in disk a conanfile.txt with the requires, options, and imports
  # specified as arguments
  # This will be considered as temporary file, generated in CMAKE_CURRENT_BINARY_DIR)
  parse_arguments(${ARGV})
  set(_FN "${CMAKE_CURRENT_BINARY_DIR}/conanfile.txt")

  file(WRITE ${_FN} "[generators]\ncmake\n\n[requires]\n")
  foreach(ARG ${ARGUMENTS_REQUIRES})
    file(APPEND ${_FN} ${ARG} "\n")
  endforeach()

  file(APPEND ${_FN} ${ARG} "\n[options]\n")
  foreach(ARG ${ARGUMENTS_OPTIONS})
    file(APPEND ${_FN} ${ARG} "\n")
  endforeach()

  file(APPEND ${_FN} ${ARG} "\n[imports]\n")
  foreach(ARG ${ARGUMENTS_IMPORTS})
    file(APPEND ${_FN} ${ARG} "\n")
  endforeach()
endfunction()


macro(conan_load_buildinfo)
    if(CONAN_CMAKE_MULTI)
      set(_CONANBUILDINFO conanbuildinfo_multi.cmake)
    else()
      set(_CONANBUILDINFO conanbuildinfo.cmake)
    endif()
    # Checks for the existence of conanbuildinfo.cmake, and loads it
    # important that it is macro, so variables defined at parent scope
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${_CONANBUILDINFO}")
      message(STATUS "Conan: Loading ${_CONANBUILDINFO}")
      include(${CMAKE_CURRENT_BINARY_DIR}/${_CONANBUILDINFO})
    else()
      message(FATAL_ERROR "${_CONANBUILDINFO} doesn't exist in ${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endmacro()


macro(conan_cmake_run)
    parse_arguments(${ARGV})

    if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE AND NOT CONAN_EXPORTED)
        set(CONAN_CMAKE_MULTI ON)
        message(STATUS "Conan: Using cmake-multi generator")
    else()
        set(CONAN_CMAKE_MULTI OFF)
    endif()
    if(NOT CONAN_EXPORTED)
        conan_cmake_setup_conanfile(${ARGV})
        if(CONAN_CMAKE_MULTI)
            foreach(CMAKE_BUILD_TYPE "Release" "Debug")
                conan_cmake_settings(settings ${ARGV})
                conan_cmake_install(SETTINGS ${settings} ${ARGV})
            endforeach()
            set(CMAKE_BUILD_TYPE)
        else()
            conan_cmake_settings(settings ${ARGV})
            conan_cmake_install(SETTINGS ${settings} ${ARGV})
        endif()
    endif()

    conan_load_buildinfo()

    if(ARGUMENTS_BASIC_SETUP)
        foreach(_option CMAKE_TARGETS KEEP_RPATHS NO_OUTPUT_DIRS)
            if(ARGUMENTS_${_option})
                if(${_option} STREQUAL "CMAKE_TARGETS")
                    list(APPEND _setup_options "TARGETS")
                else()
                    list(APPEND _setup_options ${_option})
                endif()
            endif()
        endforeach()
        conan_basic_setup(${_setup_options})
    endif()
endmacro()
