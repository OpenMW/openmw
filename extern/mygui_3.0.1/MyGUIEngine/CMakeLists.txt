set (PROJECT_NAME MyGUIEngine)

project( ${PROJECT_NAME} )

include(${PROJECT_NAME}.list)

include_directories(include)
if (MYGUI_USE_FREETYPE)
	include_directories(
		${FREETYPE_INCLUDE_DIRS}
	)
	add_definitions(-DMYGUI_USE_FREETYPE)
endif ()

add_definitions(-DMYGUI_BUILD)

if (WIN32)
	add_definitions(-DNOMINMAX)
endif ()

add_library(${PROJECT_NAME} ${MYGUI_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
if (MYGUI_USE_FREETYPE)
	target_link_libraries(${PROJECT_NAME}
		${FREETYPE_LIBRARIES}
	)
endif()

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS} uuid)

if (MINGW)
  target_link_libraries(${PROJECT_NAME} libwinmm.a)
endif (MINGW)
