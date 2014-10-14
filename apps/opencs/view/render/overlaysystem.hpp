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
                if(mOverlaySystem) delete mOverlaySystem;
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

            void destroy() {
                delete mOverlaySystem;
                mOverlaySystem = NULL;
            }
    };
}

#endif // OPENCS_VIEW_OVERLAYSYSTEM_H
