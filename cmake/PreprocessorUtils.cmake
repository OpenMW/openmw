#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

macro(get_preprocessor_entry CONTENTS KEYWORD VARIABLE)
  string(REGEX MATCH
    "# *define +${KEYWORD} +((\"([^\n]*)\")|([^ \n]*))"
    PREPROC_TEMP_VAR
    ${${CONTENTS}}
  )
  if (CMAKE_MATCH_3)
    set(${VARIABLE} ${CMAKE_MATCH_3})
  else ()
    set(${VARIABLE} ${CMAKE_MATCH_4})
  endif ()
endmacro()

macro(has_preprocessor_entry CONTENTS KEYWORD VARIABLE)
  string(REGEX MATCH
    "\n *# *define +(${KEYWORD})"
    PREPROC_TEMP_VAR
    ${${CONTENTS}}
  )
  if (CMAKE_MATCH_1)
    set(${VARIABLE} TRUE)
  else ()
    set(${VARIABLE} FALSE)
  endif ()
endmacro()

macro(replace_preprocessor_entry VARIABLE KEYWORD NEW_VALUE)
  string(REGEX REPLACE 
    "(// *)?# *define +${KEYWORD} +[^ \n]*"
        "#define ${KEYWORD} ${NEW_VALUE}"
        ${VARIABLE}_TEMP
        ${${VARIABLE}}
  )
  set(${VARIABLE} ${${VARIABLE}_TEMP})  
endmacro()

macro(set_preprocessor_entry VARIABLE KEYWORD ENABLE)
  if (${ENABLE})
    set(TMP_REPLACE_STR "#define ${KEYWORD}")
  else ()
    set(TMP_REPLACE_STR "// #define ${KEYWORD}")
  endif ()
  string(REGEX REPLACE 
    "(// *)?# *define +${KEYWORD} *\n"
        ${TMP_REPLACE_STR}
        ${VARIABLE}_TEMP
        ${${VARIABLE}}
  )
  set(${VARIABLE} ${${VARIABLE}_TEMP})  
endmacro()


# get_version_from_n_defines(result_version_name header_path [list of defines...])
#
# get_version_from_n_defines(MyPackage_VERSION /Header/Path/HeaderName.h
#     MYPACKAGE_VERSION_MAJOR
#     MYPACKAGE_VERSION_MINOR
# )
# Function call will get the values of defines MYPACKAGE_VERSION_MAJOR & MYPACKAGE_VERSION_MINOR
#  from header and set "${MYPACKAGE_VERSION_MAJOR}.${MYPACKAGE_VERSION_MINOR}" into MyPackage_VERSION
#

function(get_version_from_n_defines OUT_VAR HEADER_PATH)
    if (NOT EXISTS ${HEADER_PATH})
        message(FATAL_ERROR "Unable to find '${HEADER_PATH}'")
        return()
    endif ()
    file(READ ${HEADER_PATH} _CONTENT)
    unset(_DEFINES_LIST)
    foreach (_DEFINE_NAME ${ARGN})
        get_preprocessor_entry(_CONTENT ${_DEFINE_NAME} _DEFINE_VALUE)
        list(APPEND _DEFINES_LIST ${_DEFINE_VALUE})
    endforeach()
    string(REPLACE ";" "." _VERSION "${_DEFINES_LIST}")
    set(${OUT_VAR} "${_VERSION}" PARENT_SCOPE)
endfunction()
