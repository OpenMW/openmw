# Taken from vcpkg's z_vcpkg_escape_regex_control_characters
# MIT licence
# Someone should check this preamble is up to our usual high standards of legal compliance

function(escape_regex_control_characters out_var string)
    if(ARGC GREATER "2")
        message(FATAL_ERROR "escape_regex_control_characters passed extra arguments: ${ARGN}")
    endif()
    # OpenMW should use CMake 4.2's string(REGEX QUOTE out_var input) if available
    # uses | instead of [] to avoid confusion; additionally, CMake doesn't support `]` in a `[]`
    string(REGEX REPLACE [[\[|\]|\(|\)|\.|\+|\*|\^|\\|\$|\?|\|]] [[\\\0]] escaped_content "${string}")
    set("${out_var}" "${escaped_content}" PARENT_SCOPE)
endfunction()