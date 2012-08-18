set(OICS_LIBRARY "oics")

# Sources

set(OICS_SOURCE_FILES
	ICSChannel.cpp
	ICSControl.cpp
	ICSInputControlSystem.cpp
	ICSInputControlSystem_keyboard.cpp
	ICSInputControlSystem_mouse.cpp
	ICSInputControlSystem_joystick.cpp
	tinyxml.cpp
	tinyxmlparser.cpp
	tinyxmlerror.cpp
    tinystr.cpp
)

add_library(${OICS_LIBRARY} STATIC ${OICS_SOURCE_FILES})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
