# Copyright (c) 2014 Jarryd Beck
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
if (CMAKE_VERSION VERSION_GREATER 3.10 OR CMAKE_VERSION VERSION_EQUAL 3.10)
    # Use include_guard() added in cmake 3.10
    include_guard()
endif()

include(CMakePackageConfigHelpers)

function(cxxopts_getversion version_arg)
    # Parse the current version from the cxxopts header
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/cxxopts.hpp" cxxopts_version_defines
        REGEX "#define CXXOPTS__VERSION_(MAJOR|MINOR|PATCH)")
    foreach(ver ${cxxopts_version_defines})
        if(ver MATCHES "#define CXXOPTS__VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
            set(CXXOPTS__VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
        endif()
    endforeach()
    set(VERSION ${CXXOPTS__VERSION_MAJOR}.${CXXOPTS__VERSION_MINOR}.${CXXOPTS__VERSION_PATCH})

    # Give feedback to the user. Prefer DEBUG when available since large projects tend to have a lot
    # going on already
    if (CMAKE_VERSION VERSION_GREATER 3.15 OR CMAKE_VERSION VERSION_EQUAL 3.15)
        message(DEBUG "cxxopts version ${VERSION}")
    else()
        message(STATUS "cxxopts version ${VERSION}")
    endif()

    # Return the information to the caller
    set(${version_arg} ${VERSION} PARENT_SCOPE)
endfunction()

# Optionally, enable unicode support using the ICU library
function(cxxopts_use_unicode)
    find_package(PkgConfig)
    pkg_check_modules(ICU REQUIRED icu-uc)

    target_link_libraries(cxxopts INTERFACE ${ICU_LDFLAGS})
    target_compile_options(cxxopts INTERFACE ${ICU_CFLAGS})
    target_compile_definitions(cxxopts INTERFACE CXXOPTS_USE_UNICODE)
endfunction()

# Request C++11 without gnu extension for the whole project and enable more warnings
macro(cxxopts_set_cxx_standard)
    if (CXXOPTS_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD ${CXXOPTS_CXX_STANDARD})
    else()
        set(CMAKE_CXX_STANDARD 11)
    endif()

    set(CMAKE_CXX_EXTENSIONS OFF)
endmacro()

# Helper function to enable warnings
function(cxxopts_enable_warnings)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
          if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 5.0)
            set(COMPILER_SPECIFIC_FLAGS "-Wsuggest-override")
          endif()
      endif()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wshadow -Weffc++ -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self -Wconversion -Wno-sign-conversion ${COMPILER_SPECIFIC_FLAGS}")
    endif()

    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
endfunction()

# Helper function to ecapsulate install logic
function(cxxopts_install_logic)
    if(CMAKE_LIBRARY_ARCHITECTURE)
        string(REPLACE "/${CMAKE_LIBRARY_ARCHITECTURE}" "" CMAKE_INSTALL_LIBDIR_ARCHIND "${CMAKE_INSTALL_LIBDIR}")
    else()
        # On some systems (e.g. NixOS), `CMAKE_LIBRARY_ARCHITECTURE` can be empty
        set(CMAKE_INSTALL_LIBDIR_ARCHIND "${CMAKE_INSTALL_LIBDIR}")
    endif()
    set(CXXOPTS_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR_ARCHIND}/cmake/cxxopts" CACHE STRING "Installation directory for cmake files, relative to ${CMAKE_INSTALL_PREFIX}.")
    set(version_config "${PROJECT_BINARY_DIR}/cxxopts-config-version.cmake")
    set(project_config "${PROJECT_BINARY_DIR}/cxxopts-config.cmake")
    set(targets_export_name cxxopts-targets)
    set(PackagingTemplatesDir "${PROJECT_SOURCE_DIR}/packaging")


    if(${CMAKE_VERSION} VERSION_GREATER "3.14")
        set(OPTIONAL_ARCH_INDEPENDENT "ARCH_INDEPENDENT")
    endif()

    # Generate the version, config and target files into the build directory.
    write_basic_package_version_file(
        ${version_config}
        VERSION ${VERSION}
        COMPATIBILITY AnyNewerVersion
        ${OPTIONAL_ARCH_INDEPENDENT}
    )
    configure_package_config_file(
        ${PackagingTemplatesDir}/cxxopts-config.cmake.in
        ${project_config}
        INSTALL_DESTINATION ${CXXOPTS_CMAKE_DIR})
    export(TARGETS cxxopts NAMESPACE cxxopts::
        FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

    # Install version, config and target files.
    install(
        FILES ${project_config} ${version_config}
        DESTINATION ${CXXOPTS_CMAKE_DIR})
    install(EXPORT ${targets_export_name} DESTINATION ${CXXOPTS_CMAKE_DIR}
        NAMESPACE cxxopts::)

    # Install the header file and export the target
    install(TARGETS cxxopts EXPORT ${targets_export_name} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES ${PROJECT_SOURCE_DIR}/include/cxxopts.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "cxxopt developers")
    set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
    set(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
    set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
    set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
    set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_MAINTAINER}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

    set(CPACK_DEBIAN_PACKAGE_NAME "lib${PROJECT_NAME}-dev")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6-dev")
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS "cmake, pkg-config, pkg-conf")

    set(CPACK_RPM_PACKAGE_NAME "lib${PROJECT_NAME}-devel")
    set(CPACK_RPM_PACKAGE_SUGGESTS "${CPACK_DEBIAN_PACKAGE_SUGGESTS}")

    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_NSIS_COMPONENT_INSTALL ON)
    set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")

    set(PKG_CONFIG_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc")
    configure_file("${PackagingTemplatesDir}/pkgconfig.pc.in" "${PKG_CONFIG_FILE_NAME}" @ONLY)
    install(FILES "${PKG_CONFIG_FILE_NAME}"
            DESTINATION "${CMAKE_INSTALL_LIBDIR_ARCHIND}/pkgconfig"
    )

    include(CPack)
endfunction()
