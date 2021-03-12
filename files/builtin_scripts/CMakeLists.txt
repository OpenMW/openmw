file(GLOB_RECURSE BUILTIN_SCRIPTS LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*")

foreach (f ${BUILTIN_SCRIPTS})
    if (NOT ("CMakeLists.txt" STREQUAL "${f}"))
        copy_resource_file("${CMAKE_CURRENT_SOURCE_DIR}/${f}" "${OpenMW_BINARY_DIR}" "resources/vfs/${f}")
    endif()
endforeach (f)

