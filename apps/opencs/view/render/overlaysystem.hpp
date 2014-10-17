#ifndef OPENCS_VIEW_OVERLAYSYSTEM_H
#define OPENCS_VIEW_OVERLAYSYSTEM_H

namespace Ogre
{
    class OverlaySystem;
}

namespace CSVRender
{
    class OverlaySystem
    {
            Ogre::OverlaySystem *mOverlaySystem;
            static OverlaySystem *mOverlaySystemInstance;

        public:

            OverlaySystem();
            ~OverlaySystem();
            static OverlaySystem &instance();

            Ogre::OverlaySystem *get();
    };
}

#endif // OPENCS_VIEW_OVERLAYSYSTEM_H
