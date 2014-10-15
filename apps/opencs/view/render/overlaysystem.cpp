#include "overlaysystem.hpp"

#include <OgreOverlaySystem.h>

namespace CSVRender
{
    OverlaySystem *OverlaySystem::mOverlaySystemInstance = 0;

    OverlaySystem::OverlaySystem() : mOverlaySystem(NULL)
    {
        assert(!mOverlaySystemInstance);
        mOverlaySystemInstance = this;
    }

    OverlaySystem::~OverlaySystem()
    {
        if(mOverlaySystem)
            delete mOverlaySystem;
    }

    OverlaySystem &OverlaySystem::instance()
    {
        assert(mOverlaySystemInstance);
        return *mOverlaySystemInstance;
    }

    Ogre::OverlaySystem *OverlaySystem::get()
    {
        if(!mOverlaySystem)
            mOverlaySystem = new Ogre::OverlaySystem();
        return mOverlaySystem;
    }

    void OverlaySystem::destroy()
    {
        delete mOverlaySystem;
        mOverlaySystem = NULL;
    }
}

