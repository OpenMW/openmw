set(OENGINE_OGRE
    ogre/renderer.cpp
    ogre/lights.cpp
    ogre/selectionbuffer.cpp
)

set(OENGINE_GUI
    gui/loglistener.cpp
    gui/manager.cpp
    gui/layout.cpp
)

set(OENGINE_BULLET
    bullet/BtOgre.cpp
    bullet/BtOgreExtras.h
    bullet/BtOgreGP.h
    bullet/BtOgrePG.h
    bullet/physic.cpp
    bullet/physic.hpp
    bullet/BulletShapeLoader.cpp
    bullet/BulletShapeLoader.h
    bullet/trace.cpp
    bullet/trace.h
)

set(OENGINE_MISC
    misc/rng.cpp
    misc/rng.hpp
)

set(OENGINE_ALL ${OENGINE_OGRE} ${OENGINE_GUI} ${OENGINE_BULLET} ${OENGINE_MISC})

set(OENGINE_LIBRARY "oengine")
set(OENGINE_LIBRARY ${OENGINE_LIBRARY} PARENT_SCOPE)

source_group(oengine FILES ${OENGINE_ALL})

add_library(${OENGINE_LIBRARY} STATIC ${OENGINE_ALL})
