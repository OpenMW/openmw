#ifndef OPENCS_VIEW_OVERLAYSYSTEM_H
#define OPENCS_VIEW_OVERLAYSYSTEM_H

#include <OgreOverlaySystem.h>

namespace CSVRender
{
    class OverlaySystem
    {
            Ogre::OverlaySystem *mOverlaySystem;

            OverlaySystem() {
                mOverlaySystem = new Ogre::OverlaySystem();
            }

            ~OverlaySystem() {
                delete mOverlaySystem;
            }

            OverlaySystem(OverlaySystem const&);
            void operator=(OverlaySystem const&);

        public:

            static OverlaySystem &instance() {
                static OverlaySystem mInstance;
                return mInstance;
            }

            Ogre::OverlaySystem *get() {
                return mOverlaySystem;
            }
    };
}

#endif // OPENCS_VIEW_OVERLAYSYSTEM_H
