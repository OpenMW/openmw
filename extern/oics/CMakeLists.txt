add_library(oics STATIC
    ICSChannel.cpp
    ICSControl.cpp
    ICSInputControlSystem.cpp
    ICSInputControlSystem_keyboard.cpp
    ICSInputControlSystem_mouse.cpp
    ICSInputControlSystem_joystick.cpp
    ICSPrerequisites.h
)

if(USE_SYSTEM_TINYXML)
    target_link_libraries(oics ${TinyXML_LIBRARIES})
else()
    add_library(local_tinyxml STATIC
        tinyxml.cpp
        tinyxmlparser.cpp
        tinyxmlerror.cpp
        tinystr.cpp
    )
    target_link_libraries(oics local_tinyxml)
endif()

target_link_libraries(oics SDL2::SDL2)

if (MSVC AND PRECOMPILE_HEADERS_WITH_MSVC)
    target_precompile_headers(oics PUBLIC <algorithm> <string>)
endif()
