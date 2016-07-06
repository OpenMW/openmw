set(OICS_LIBRARY "oics")

# Sources

set(OICS_SOURCE_FILES
	ICSChannel.cpp
	ICSControl.cpp
	ICSInputControlSystem.cpp
	ICSInputControlSystem_keyboard.cpp
	ICSInputControlSystem_mouse.cpp
	ICSInputControlSystem_joystick.cpp
	ICSPrerequisites.h
)

set(TINYXML_SOURCE_FILES
	tinyxml.cpp
	tinyxmlparser.cpp
	tinyxmlerror.cpp
	tinystr.cpp	
)

if(USE_SYSTEM_TINYXML)
    add_library(${OICS_LIBRARY} STATIC ${OICS_SOURCE_FILES})
    target_link_libraries(${OICS_LIBRARY} ${TINYXML_LIBRARIES})
else()
    add_library(${OICS_LIBRARY} STATIC
    ${OICS_SOURCE_FILES}
    ${TINYXML_SOURCE_FILES})
endif()

# Does this do anything?
link_directories(${CMAKE_CURRENT_BINARY_DIR})
