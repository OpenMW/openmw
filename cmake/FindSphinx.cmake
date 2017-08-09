# - This module looks for Sphinx
# Find the Sphinx documentation generator
#
# This modules defines
#  Sphinx_EXECUTABLE
#  Sphinx_FOUND
#  function Sphinx_add_target
#  function Sphinx_add_targets
#

include(FindPackageHandleStandardArgs)
include(CMakeParseArguments)

set(_sphinx_names sphinx-build)
foreach(_version 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0 1.6 1.5)
    list(APPEND _sphinx_names sphinx-build-${_version})
endforeach()

find_program(Sphinx_EXECUTABLE
    NAMES ${_sphinx_names}
    PATHS
        /usr/bin
        /usr/local/bin
        /opt/local/bin
    DOC "Sphinx documentation generator"
)
mark_as_advanced(Sphinx_EXECUTABLE)


FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sphinx
    FOUND_VAR Sphinx_FOUND
    REQUIRED_VARS Sphinx_EXECUTABLE
)

option( SPHINX_HTML_OUTPUT "Build a single HTML with the whole content." ON )
option( SPHINX_DIRHTML_OUTPUT "Build HTML pages, but with a single directory per document." OFF )
option( SPHINX_HTMLHELP_OUTPUT "Build HTML pages with additional information for building a documentation collection in htmlhelp." OFF )
option( SPHINX_QTHELP_OUTPUT "Build HTML pages with additional information for building a documentation collection in qthelp." OFF )
option( SPHINX_DEVHELP_OUTPUT "Build HTML pages with additional information for building a documentation collection in devhelp." OFF )
option( SPHINX_EPUB_OUTPUT "Build HTML pages with additional information for building a documentation collection in epub." OFF )
option( SPHINX_LATEX_OUTPUT "Build LaTeX sources that can be compiled to a PDF document using pdflatex." OFF )
option( SPHINX_MAN_OUTPUT "Build manual pages in groff format for UNIX systems." OFF )
option( SPHINX_TEXT_OUTPUT "Build plain text files." OFF )

function(Sphinx_add_target target_name builder conf source destination)
    add_custom_target( ${target_name} ALL
        COMMAND ${Sphinx_EXECUTABLE} -b ${builder} -c ${conf} ${source} ${destination}
        DEPENDS ${conf}/conf.py
        COMMENT "Generating sphinx documentation: ${builder}"
    )

    set_property(DIRECTORY APPEND PROPERTY
        ADDITIONAL_MAKE_CLEAN_FILES ${destination}
    )
endfunction()

# Usage:
#
# Sphinx_add_targets(NAME <base_targets_name>
#   SRC_DIR <path_to_doc>/
#   DST_DIR <path_to_output_result>/
#   CONFIG_DIR <path_to_conf.py>/
#   [DEPENDENCIES dep1 dep2 dep3 ...]
# )
function(Sphinx_add_targets)
    set(options )
    set(one_value_keywords NAME SRC_DIR DST_DIR CONFIG_DIR)
    set(multi_value_keywords DEPENDENCIES)
    CMAKE_PARSE_ARGUMENTS(OPT "${options}" "${one_value_keywords}" "${multi_value_keywords}" ${ARGN})

    if (NOT OPT_NAME OR NOT OPT_SRC_DIR OR NOT OPT_DST_DIR OR NOT OPT_CONFIG_DIR)
        message(FATAL_ERROR "Arguments NAME, SRC_DIR, DST_DIR, CONFIG_DIR are required!")
    endif()

    foreach(_generator html dirhtml qthelp devhelp epub latex man text linkcheck)
        string(TOUPPER ${_generator} _generator_uc)
        if (SPHINX_${_generator_uc}_OUTPUT)
            Sphinx_add_target(${OPT_NAME}_${_generator} ${_generator} ${OPT_CONFIG_DIR} ${OPT_SRC_DIR} ${OPT_DST_DIR}/${_generator}/)
            if (OPT_DEPENDENCIES)
                add_dependencies(${OPT_NAME}_${_generator} ${OPT_DEPENDENCIES})
            endif()
        endif()
    endforeach()
endfunction()
