project(resources)

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/water/* "${OpenMW_BINARY_DIR}/resources/water/")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/gbuffer/* "${OpenMW_BINARY_DIR}/resources/gbuffer/")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/shadows/* "${OpenMW_BINARY_DIR}/resources/shadows/")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/materials/* "${OpenMW_BINARY_DIR}/resources/materials/")
