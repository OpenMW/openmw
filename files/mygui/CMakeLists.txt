
# Copy resource files into the build directory
set(SDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DDIR ${OpenMW_BINARY_DIR}/resources/mygui)

copy_all_files(${SDIR}/* ${DDIR})
