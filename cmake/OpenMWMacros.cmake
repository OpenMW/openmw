
macro (add_openmw_dir dir)
set (files)
foreach (u ${ARGN})
list (APPEND files "${dir}/${u}")
list (APPEND OPENMW_FILES "${dir}/${u}")
endforeach (u)
source_group ("apps\\openmw\\${dir}" FILES ${files})
endmacro (add_openmw_dir)
