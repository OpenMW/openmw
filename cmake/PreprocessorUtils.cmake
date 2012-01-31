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
  
