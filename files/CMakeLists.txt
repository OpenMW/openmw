project(resources)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/caustic_0.png "${OpenMW_BINARY_DIR}/resources/water/caustic_0.png" COPYONLY) 
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/underwater.cg "${OpenMW_BINARY_DIR}/resources/water/underwater.cg" COPYONLY) 
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/perlinvolume.dds "${OpenMW_BINARY_DIR}/resources/water/perlinvolume.dds" COPYONLY) 
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/water.compositor "${OpenMW_BINARY_DIR}/resources/water/water.compositor" COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/water.material "${OpenMW_BINARY_DIR}/resources/water/water.material" COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/WaterNormal2.tga "${OpenMW_BINARY_DIR}/resources/water/WaterNormal2.tga" COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/water/water.cg "${OpenMW_BINARY_DIR}/resources/water/water.cg" COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer.cg "${OpenMW_BINARY_DIR}/resources/gbuffer/gbuffer.cg" COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer.material "${OpenMW_BINARY_DIR}/resources/gbuffer/gbuffer.material" COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/gbuffer.compositor "${OpenMW_BINARY_DIR}/resources/gbuffer/gbuffer.compositor" COPYONLY)
