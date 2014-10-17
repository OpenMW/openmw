#include "overlaysystem.hpp"

#include <cassert>

#include <OgreOverlaySystem.h>

namespace CSVRender
{
    OverlaySystem *OverlaySystem::mOverlaySystemInstance = 0;

    OverlaySystem::OverlaySystem()
    {
        assert(!mOverlaySystemInstance);
        mOverlaySystemInstance = this;
        mOverlaySystem = new Ogre::OverlaySystem();
    }

    OverlaySystem::~OverlaySystem()
    {
        delete mOverlaySystem;
    }

    OverlaySystem &OverlaySystem::instance()
    {
        assert(mOverlaySystemInstance);
        return *mOverlaySystemInstance;
    }

    Ogre::OverlaySystem *OverlaySystem::get()
    {
        return mOverlaySystem;
    }
}

