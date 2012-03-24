#ifndef _GAME_OCCLUSION_QUERY_H
#define _GAME_OCCLUSION_QUERY_H

#include <OgreHardwareOcclusionQuery.h>

namespace MWRender
{
    ///
    /// \brief Implements hardware occlusion queries on the GPU
    ///
    class OcclusionQuery
    {
    public:
        OcclusionQuery();

        bool supported();
        ///< returns true if occlusion queries are supported on the user's hardware

    private:
        Ogre::HardwareOcclusionQuery* mSunTotalAreaQuery;
        Ogre::HardwareOcclusionQuery* mSunVisibleAreaQuery;

        bool mSupported;
    };
}

#endif
