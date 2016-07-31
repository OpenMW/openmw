# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIR ${OpenMW_BINARY_DIR}/resources/shaders)

set(SHADER_FILES
    water_vertex.glsl
    water_fragment.glsl
    water_nm.png
    objects_vertex.glsl
    objects_fragment.glsl
    terrain_vertex.glsl
    terrain_fragment.glsl
    lighting.glsl
    parallax.glsl
)

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR} ${DDIR} "${SHADER_FILES}")
