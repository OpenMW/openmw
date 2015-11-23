# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIR ${OpenMW_BINARY_DIR}/resources/shaders)
if (OPENGLES)
    set(SHADER_FILES
        watergles_vertex.glsl
        watergles_fragment.glsl
        water_nm.png
    )
else()
   set(SHADER_FILES
        water_vertex.glsl
        water_fragment.glsl
        water_nm.png
    )
endif()
copy_all_files(${CMAKE_CURRENT_SOURCE_DIR} ${DDIR} "${SHADER_FILES}")
