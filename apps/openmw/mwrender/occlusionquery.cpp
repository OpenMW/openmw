#include "occlusionquery.hpp"

#include <OgreRenderSystem.h>
#include <OgreRoot.h>

using namespace MWRender;
using namespace Ogre;

OcclusionQuery::OcclusionQuery() :
    mSunTotalAreaQuery(0), mSunVisibleAreaQuery(0)
{
    try {
        RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();

        mSunTotalAreaQuery = renderSystem->createHardwareOcclusionQuery();
        mSunVisibleAreaQuery = renderSystem->createHardwareOcclusionQuery();

        mSupported = (mSunTotalAreaQuery != 0) && (mSunVisibleAreaQuery != 0);
    }
    catch (Ogre::Exception e)
    {
        mSupported = false;
    }

    if (!mSupported)
        std::cout << "Hardware occlusion queries not supported." << std::endl;
}
