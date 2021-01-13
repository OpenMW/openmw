# Version 2.2
# Public Domain, originally written by Lasse Kärkkäinen <tronic>
# Maintained at https://github.com/Tronic/cmake-modules
# Please send your improvements as pull requests on Github.

include(CMakeParseArguments)

# Find another package and make it a dependency of the current package.
# This also automatically forwards the "REQUIRED" argument.
# Usage: libfind_package(<prefix> <another package> [extra args to find_package])
macro (libfind_package PREFIX PKG)
  set(${PREFIX}_args ${PKG} ${ARGN})
  if (${PREFIX}_FIND_REQUIRED)
    set(${PREFIX}_args ${${PREFIX}_args} REQUIRED)
  endif()
  find_package(${${PREFIX}_args})
  set(${PREFIX}_DEPENDENCIES ${${PREFIX}_DEPENDENCIES};${PKG})
  unset(${PREFIX}_args)
endmacro()

# A simple wrapper to make pkg-config searches a bit easier.
# Works the same as CMake's internal pkg_search_module but is always quiet.
macro (libfind_pkg_search_module)
  find_package(PkgConfig QUIET)
  if (PKG_CONFIG_FOUND)
    pkg_search_module(${ARGN} QUIET)
  endif()
endmacro()

# Avoid useless copy&pasta by doing what most simple libraries do anyway:
# pkg-config, find headers, find library.
# Usage: libfind_pkg_detect(<prefix> <pkg-config args> FIND_PATH <name> [other args] FIND_LIBRARY <name> [other args])
# E.g. libfind_pkg_detect(SDL2 sdl2 FIND_PATH SDL.h PATH_SUFFIXES SDL2 FIND_LIBRARY SDL2)
function (libfind_pkg_detect PREFIX)
  # Parse arguments
  set(argname pkgargs)
  foreach (i ${ARGN})
    if ("${i}" STREQUAL "FIND_PATH")
      set(argname pathargs)
    elseif ("${i}" STREQUAL "FIND_LIBRARY")
      set(argname libraryargs)
    else()
      set(${argname} ${${argname}} ${i})
    endif()
  endforeach()
  if (NOT pkgargs)
    message(FATAL_ERROR "libfind_pkg_detect requires at least a pkg_config package name to be passed.")
  endif()
  # Find library
  libfind_pkg_search_module(${PREFIX}_PKGCONF ${pkgargs})
  if (pathargs)
    find_path(${PREFIX}_INCLUDE_DIR NAMES ${pathargs} HINTS ${${PREFIX}_PKGCONF_INCLUDE_DIRS})
  endif()
  if (libraryargs)
    find_library(${PREFIX}_LIBRARY NAMES ${libraryargs} HINTS ${${PREFIX}_PKGCONF_LIBRARY_DIRS})
  endif()
endfunction()

# libfind_header_path(<PREFIX> [PATHS <path> [<path> ...]] NAMES <name> [name ...] VAR <out_var> [QUIET])
# Get fullpath of the first found header looking inside <PREFIX>_INCLUDE_DIR or in the given PATHS
# Usage: libfind_header_path(Foobar NAMES foobar/version.h VAR filepath)
function (libfind_header_path PREFIX)
  set(options QUIET)
  set(one_value_keywords VAR PATH)
  set(multi_value_keywords NAMES PATHS)
  CMAKE_PARSE_ARGUMENTS(OPT "${options}" "${one_value_keywords}" "${multi_value_keywords}" ${ARGN})
  if (NOT OPT_VAR OR NOT OPT_NAMES)
    message(FATAL_ERROR "Arguments VAR, NAMES are required!")
  endif()
  if (OPT_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Calling function with unused arguments '${OPT_UNPARSED_ARGUMENTS}'!")
  endif()
  if (OPT_QUIET OR ${PREFIX}_FIND_QUIETLY)
    set(quiet TRUE)
  endif()
  set(paths ${OPT_PATHS} ${PREFIX}_INCLUDE_DIR)

  foreach(name ${OPT_NAMES})
    foreach(path ${paths})
      set(filepath "${${path}}/${name}")
      # check for existance
      if (EXISTS ${filepath})
        set(${OPT_VAR} ${filepath} PARENT_SCOPE) # export path
        return()
      endif()
    endforeach()
  endforeach()

  # report error if not found
  set(${OPT_VAR} NOTFOUND PARENT_SCOPE)
  if (NOT quiet)
    message(AUTHOR_WARNING "Unable to find '${OPT_NAMES}'")
  endif()
endfunction()

# libfind_version_n_header(<PREFIX>
#  NAMES <name> [<name> ...]
#  DEFINES <define> [<define> ...] | CONSTANTS <const> [<const> ...]
#  [PATHS <path> [<path> ...]]
#  [QUIET]
# )
# Collect all defines|constants from a header inside <PREFIX>_INCLUDE_DIR or in the given PATHS
#  output stored to <PREFIX>_VERSION.
# This function does nothing if the version variable is already defined.
# Usage: libfind_version_n_header(Foobar NAMES foobar/version.h DEFINES FOOBAR_VERSION_MAJOR FOOBAR_VERSION_MINOR)
function (libfind_version_n_header PREFIX)
  # Skip processing if we already have a version
  if (${PREFIX}_VERSION)
    return()
  endif()

  set(options QUIET)
  set(one_value_keywords )
  set(multi_value_keywords NAMES PATHS DEFINES CONSTANTS)
  CMAKE_PARSE_ARGUMENTS(OPT "${options}" "${one_value_keywords}" "${multi_value_keywords}" ${ARGN})
  if (NOT OPT_NAMES OR (NOT OPT_DEFINES AND NOT OPT_CONSTANTS))
    message(FATAL_ERROR "Arguments NAMES, DEFINES|CONSTANTS are required!")
  endif()
  if (OPT_DEFINES AND OPT_CONSTANTS)
    message(FATAL_ERROR "Either DEFINES or CONSTANTS must be set!")
  endif()
  if (OPT_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Calling function with unused arguments '${OPT_UNPARSED_ARGUMENTS}'!")
  endif()
  if (OPT_QUIET OR ${PREFIX}_FIND_QUIETLY)
    set(quiet TRUE)
    set(force_quiet "QUIET") # to propagate argument QUIET
  endif()

  # Read the header
  libfind_header_path(${PREFIX} NAMES ${OPT_NAMES} PATHS ${OPT_PATHS} VAR filename ${force_quiet})
  if (NOT filename)
    return()
  endif()
  file(READ "${filename}" header)
  # Parse for version number
  unset(version_parts)
  foreach(define_name ${OPT_DEFINES})
    string(REGEX MATCH "# *define +${define_name} +((\"([^\n]*)\")|([^ \n]*))" match "${header}")
    # No regex match?
    if (NOT match OR match STREQUAL header)
      if (NOT quiet)
        message(AUTHOR_WARNING "Unable to find \#define ${define_name} \"<version>\" from ${filename}")
      endif()
      return()
    else()
      list(APPEND version_parts "${CMAKE_MATCH_3}${CMAKE_MATCH_4}")
    endif()
  endforeach()
  foreach(constant_name ${OPT_CONSTANTS})
    string(REGEX MATCH "${constant_name} *= *((\"([^\;]*)\")|([^ \;]*))" match "${header}")
    # No regex match?
    if (NOT match OR match STREQUAL header)
      if (NOT quiet)
        message(AUTHOR_WARNING "Unable to find ${constant_name} = \"<version>\" from ${filename}")
      endif()
      return()
    else()
      list(APPEND version_parts "${CMAKE_MATCH_3}${CMAKE_MATCH_4}")
    endif()
  endforeach()

  # Export the version string
  string(REPLACE ";" "." version "${version_parts}")
  set(${PREFIX}_VERSION "${version}" PARENT_SCOPE)
endfunction()

# libfind_version_header(<PREFIX> <HEADER> <DEFINE_NAME> [PATHS <path> [<path> ...]] [QUIET])
# Extracts a version #define from a version.h file, output stored to <PREFIX>_VERSION.
# This function does nothing if the version variable is already defined.
# Usage: libfind_version_header(Foobar foobar/version.h FOOBAR_VERSION_STR)
function (libfind_version_header PREFIX VERSION_H DEFINE_NAME)
  # Skip processing if we already have a version
  if (${PREFIX}_VERSION)
    return()
  endif()

  set(options QUIET)
  set(one_value_keywords )
  set(multi_value_keywords PATHS)
  CMAKE_PARSE_ARGUMENTS(OPT "${options}" "${one_value_keywords}" "${multi_value_keywords}" ${ARGN})
  if (OPT_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Calling function with unused arguments '${OPT_UNPARSED_ARGUMENTS}'!")
  endif()
  if (OPT_QUIET OR ${PREFIX}_FIND_QUIETLY)
    set(force_quiet "QUIET") # to propagate argument QUIET
  endif()

  libfind_version_n_header(${PREFIX} NAMES ${VERSION_H} PATHS ${OPT_PATHS} DEFINES ${DEFINE_NAME} ${force_quiet})
  set(${PREFIX}_VERSION "${${PREFIX}_VERSION}" PARENT_SCOPE)
endfunction()

# Do the final processing once the paths have been detected.
# If include dirs are needed, ${PREFIX}_PROCESS_INCLUDES should be set to contain
# all the variables, each of which contain one include directory.
# Ditto for ${PREFIX}_PROCESS_LIBS and library files.
# Will set ${PREFIX}_FOUND, ${PREFIX}_INCLUDE_DIRS and ${PREFIX}_LIBRARIES.
# Also handles errors in case library detection was required, etc.
function (libfind_process PREFIX)
  # Skip processing if already processed during this configuration run
  if (${PREFIX}_FOUND)
    return()
  endif()

  set(found TRUE)  # Start with the assumption that the package was found

  # Did we find any files? Did we miss includes? These are for formatting better error messages.
  set(some_files FALSE)
  set(missing_headers FALSE)

  # Shorthands for some variables that we need often
  set(quiet ${${PREFIX}_FIND_QUIETLY})
  set(required ${${PREFIX}_FIND_REQUIRED})
  set(exactver ${${PREFIX}_FIND_VERSION_EXACT})
  set(findver "${${PREFIX}_FIND_VERSION}")
  set(version "${${PREFIX}_VERSION}")

  # Lists of config option names (all, includes, libs)
  unset(configopts)
  set(includeopts ${${PREFIX}_PROCESS_INCLUDES})
  set(libraryopts ${${PREFIX}_PROCESS_LIBS})

  # Process deps to add to
  foreach (i ${PREFIX} ${${PREFIX}_DEPENDENCIES})
    if (DEFINED ${i}_INCLUDE_OPTS OR DEFINED ${i}_LIBRARY_OPTS)
      # The package seems to export option lists that we can use, woohoo!
      list(APPEND includeopts ${${i}_INCLUDE_OPTS})
      list(APPEND libraryopts ${${i}_LIBRARY_OPTS})
    else()
      # If plural forms don't exist or they equal singular forms
      if ((NOT DEFINED ${i}_INCLUDE_DIRS AND NOT DEFINED ${i}_LIBRARIES) OR
          ({i}_INCLUDE_DIR STREQUAL ${i}_INCLUDE_DIRS AND ${i}_LIBRARY STREQUAL ${i}_LIBRARIES))
        # Singular forms can be used
        if (DEFINED ${i}_INCLUDE_DIR)
          list(APPEND includeopts ${i}_INCLUDE_DIR)
        endif()
        if (DEFINED ${i}_LIBRARY)
          list(APPEND libraryopts ${i}_LIBRARY)
        endif()
      else()
        # Oh no, we don't know the option names
        message(FATAL_ERROR "We couldn't determine config variable names for ${i} includes and libs. Aieeh!")
      endif()
    endif()
  endforeach()

  if (includeopts)
    list(REMOVE_DUPLICATES includeopts)
  endif()

  if (libraryopts)
    list(REMOVE_DUPLICATES libraryopts)
  endif()

  string(REGEX REPLACE ".*[ ;]([^ ;]*(_INCLUDE_DIRS|_LIBRARIES))" "\\1" tmp "${includeopts} ${libraryopts}")
  if (NOT tmp STREQUAL "${includeopts} ${libraryopts}")
    message(AUTHOR_WARNING "Plural form ${tmp} found in config options of ${PREFIX}. This works as before but is now deprecated. Please only use singular forms INCLUDE_DIR and LIBRARY, and update your find scripts for LibFindMacros > 2.0 automatic dependency system (most often you can simply remove the PROCESS variables entirely).")
  endif()

  # Include/library names separated by spaces (notice: not CMake lists)
  unset(includes)
  unset(libs)

  # Process all includes and set found false if any are missing
  foreach (i ${includeopts})
    list(APPEND configopts ${i})
    if (NOT "${${i}}" STREQUAL "${i}-NOTFOUND")
      list(APPEND includes "${${i}}")
    else()
      set(found FALSE)
      set(missing_headers TRUE)
    endif()
  endforeach()

  # Process all libraries and set found false if any are missing
  foreach (i ${libraryopts})
    list(APPEND configopts ${i})
    if (NOT "${${i}}" STREQUAL "${i}-NOTFOUND")
      list(APPEND libs "${${i}}")
    else()
      set (found FALSE)
    endif()
  endforeach()

  # Version checks
  if (found AND findver)
    if (NOT version)
      message(WARNING "The find module for ${PREFIX} does not provide version information, so we'll just assume that it is OK. Please fix the module or remove package version requirements to get rid of this warning.")
    elseif (version VERSION_LESS findver OR (exactver AND NOT version VERSION_EQUAL findver))
      set(found FALSE)
      set(version_unsuitable TRUE)
    endif()
  endif()

  # If all-OK, hide all config options, export variables, print status and exit
  if (found)
    foreach (i ${configopts})
      mark_as_advanced(${i})
    endforeach()
    set (${PREFIX}_INCLUDE_OPTS ${includeopts} PARENT_SCOPE)
    set (${PREFIX}_LIBRARY_OPTS ${libraryopts} PARENT_SCOPE)
    set (${PREFIX}_INCLUDE_DIRS ${includes} PARENT_SCOPE)
    set (${PREFIX}_LIBRARIES ${libs} PARENT_SCOPE)
    set (${PREFIX}_FOUND TRUE PARENT_SCOPE)
    if (NOT quiet)
      message(STATUS "Found ${PREFIX} ${${PREFIX}_VERSION}")
      if (LIBFIND_DEBUG)
        message(STATUS "  ${PREFIX}_DEPENDENCIES=${${PREFIX}_DEPENDENCIES}")
        message(STATUS "  ${PREFIX}_INCLUDE_OPTS=${includeopts}")
        message(STATUS "  ${PREFIX}_INCLUDE_DIRS=${includes}")
        message(STATUS "  ${PREFIX}_LIBRARY_OPTS=${libraryopts}")
        message(STATUS "  ${PREFIX}_LIBRARIES=${libs}")
      endif()
    endif()
    return()
  endif()

  # Format messages for debug info and the type of error
  set(vars "Relevant CMake configuration variables:\n")
  foreach (i ${configopts})
    mark_as_advanced(CLEAR ${i})
    set(val ${${i}})
    if ("${val}" STREQUAL "${i}-NOTFOUND")
      set (val "<not found>")
    elseif (val AND NOT EXISTS "${val}")
      set (val "${val}  (does not exist)")
    else()
      set(some_files TRUE)
    endif()
    set(vars "${vars}  ${i}=${val}\n")
  endforeach()
  set(vars "${vars}You may use CMake GUI, cmake -D or ccmake to modify the values. Delete CMakeCache.txt to discard all values and force full re-detection if necessary.\n")
  if (version_unsuitable)
    set(msg "${PREFIX} ${${PREFIX}_VERSION} was found but")
    if (exactver)
      set(msg "${msg} only version ${findver} is acceptable.")
    else()
      set(msg "${msg} version ${findver} is the minimum requirement.")
    endif()
  else()
    if (missing_headers)
      set(msg "We could not find development headers for ${PREFIX}. Do you have the necessary dev package installed?")
    elseif (some_files)
      set(msg "We only found some files of ${PREFIX}, not all of them. Perhaps your installation is incomplete or maybe we just didn't look in the right place?")
      if(findver)
        set(msg "${msg} This could also be caused by incompatible version (if it helps, at least ${PREFIX} ${findver} should work).")
      endif()
    else()
      set(msg "We were unable to find package ${PREFIX}.")
    endif()
  endif()

  # Fatal error out if REQUIRED
  if (required)
    set(msg "REQUIRED PACKAGE NOT FOUND\n${msg} This package is REQUIRED and you need to install it or adjust CMake configuration in order to continue building ${CMAKE_PROJECT_NAME}.")
    message(FATAL_ERROR "${msg}\n${vars}")
  endif()
  # Otherwise just print a nasty warning
  if (NOT quiet)
    message(WARNING "WARNING: MISSING PACKAGE\n${msg} This package is NOT REQUIRED and you may ignore this warning but by doing so you may miss some functionality of ${CMAKE_PROJECT_NAME}. \n${vars}")
  endif()
endfunction()

