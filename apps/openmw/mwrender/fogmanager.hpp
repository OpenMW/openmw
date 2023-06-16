#ifndef OPENMW_MWRENDER_FOGMANAGER_H
#define OPENMW_MWRENDER_FOGMANAGER_H

#include <osg/Vec4f>

namespace MWWorld
{
    class Cell;
}

namespace MWRender
{
    class FogManager
    {
    public:
        FogManager();

        void configure(float viewDistance, const MWWorld::Cell& cell);
        void configure(float viewDistance, float fogDepth, float underwaterFog, float dlFactor, float dlOffset,
            const osg::Vec4f& color);

        osg::Vec4f getFogColor(bool isUnderwater) const;
        float getFogStart(bool isUnderwater) const;
        float getFogEnd(bool isUnderwater) const;

    private:
        float mLandFogStart;
        float mLandFogEnd;
        float mUnderwaterFogStart;
        float mUnderwaterFogEnd;
        osg::Vec4f mFogColor;
        osg::Vec4f mUnderwaterColor;
        float mUnderwaterWeight;
        float mUnderwaterIndoorFog;
    };
}

#endif
