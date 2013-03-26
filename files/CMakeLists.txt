project(resources)

set(WATER_FILES
    water_nm.png
    circle.png
)

set(MATERIAL_FILES
    atmosphere.shader
    atmosphere.shaderset
    clouds.shader
    clouds.shaderset
    core.h
    moon.shader
    moon.shaderset
    objects.mat
    objects.shader
    objects.shaderset
    openmw.configuration
    quad.mat
    quad.shader
    quad.shaderset
    shadowcaster.mat
    shadowcaster.shader
    shadowcaster.shaderset
    shadows.h
    sky.mat
    stars.shader
    stars.shaderset
    sun.shader
    sun.shaderset
    terrain.shader
    terrain.shaderset
    underwater.h
    water.mat
    water.shader
    water.shaderset
    selection.mat
    selection.shader
    selection.shaderset
    watersim_heightmap.shader
    watersim_addimpulse.shader
    watersim_heighttonormal.shader
    watersim_common.h
    watersim.mat
    watersim.shaderset
)

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/water "${OpenMW_BINARY_DIR}/resources/water/" "${WATER_FILES}")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/materials "${OpenMW_BINARY_DIR}/resources/materials/" "${MATERIAL_FILES}")
