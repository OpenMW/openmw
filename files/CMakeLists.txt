project(resources)

set(WATER_FILES
    underwater_dome.mesh
    water_nm.png
)

set(GBUFFER_FILES
    gbuffer.compositor
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
    quad2.shader
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

)

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/water "${OpenMW_BINARY_DIR}/resources/water/" "${WATER_FILES}")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/gbuffer "${OpenMW_BINARY_DIR}/resources/gbuffer/" "${GBUFFER_FILES}")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/materials "${OpenMW_BINARY_DIR}/resources/materials/" "${MATERIAL_FILES}")
