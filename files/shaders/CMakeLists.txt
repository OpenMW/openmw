if (NOT DEFINED OPENMW_RESOURCES_ROOT)
    return()
endif()

# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIRRELATIVE resources/shaders)

set(SHADER_FILES
    lib/water/fresnel.glsl
    lib/water/rain_ripples.glsl
    lib/water/ripples.glsl
    lib/view/depth.glsl
    lib/luminance/constants.glsl
    lib/particle/soft.glsl
    lib/particle/occlusion.glsl
    lib/util/quickstep.glsl
    lib/util/coordinates.glsl
    lib/util/distortion.glsl
    lib/core/fragment.glsl
    lib/core/fragment.h.glsl
    lib/core/fragment_multiview.glsl
    lib/core/vertex.glsl
    lib/core/vertex.h.glsl
    lib/core/vertex_multiview.glsl
    lib/light/lighting.glsl
    lib/light/lighting_util.glsl
    lib/sky/passes.glsl
    lib/material/parallax.glsl
    lib/material/alpha.glsl
    compatibility/ripples_blobber.frag
    compatibility/ripples_simulate.frag
    compatibility/fog.glsl
    compatibility/groundcover.vert
    compatibility/groundcover.frag
    compatibility/groundcover_paging.vert
    compatibility/groundcover_paging.frag
    compatibility/water.vert
    compatibility/water.frag
    compatibility/objects.vert
    compatibility/objects.frag
    compatibility/terrain.vert
    compatibility/terrain.frag
    compatibility/shadows_vertex.glsl
    compatibility/shadows_fragment.glsl
    compatibility/shadowcasting.vert
    compatibility/shadowcasting.frag
    compatibility/vertexcolors.glsl
    compatibility/normals.glsl
    compatibility/multiview_resolve.vert
    compatibility/multiview_resolve.frag
    compatibility/depthclipped.vert
    compatibility/depthclipped.frag
    compatibility/gui.vert
    compatibility/gui.frag
    compatibility/debug.vert
    compatibility/debug.frag
    compatibility/sky.vert
    compatibility/sky.frag
    compatibility/fullscreen_tri.vert
    compatibility/fullscreen_tri.frag
    compatibility/bs/default.vert
    compatibility/bs/default.frag
    compatibility/bs/nolighting.vert
    compatibility/bs/nolighting.frag
    compatibility/luminance/resolve.frag
    compatibility/luminance/luminance.frag
    core/ripples_blobber.comp
    core/ripples_simulate.comp
    core/gui.frag
    core/gui.vert
)

copy_all_resource_files(${CMAKE_CURRENT_SOURCE_DIR} ${OPENMW_RESOURCES_ROOT} ${DDIRRELATIVE} "${SHADER_FILES}")
