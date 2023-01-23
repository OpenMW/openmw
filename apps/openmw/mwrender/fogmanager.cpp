#include "fogmanager.hpp"

#include <algorithm>

#include <components/esm/cellcommon.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/fallback/fallback.hpp>
#include <components/sceneutil/util.hpp>
#include <components/settings/settings.hpp>

namespace
{
    float DLLandFogStart;
    float DLLandFogEnd;
    float DLUnderwaterFogStart;
    float DLUnderwaterFogEnd;
    float DLInteriorFogStart;
    float DLInteriorFogEnd;
}

namespace MWRender
{
    FogManager::FogManager()
        : mLandFogStart(0.f)
        , mLandFogEnd(std::numeric_limits<float>::max())
        , mUnderwaterFogStart(0.f)
        , mUnderwaterFogEnd(std::numeric_limits<float>::max())
        , mFogColor(osg::Vec4f())
        , mDistantFog(Settings::Manager::getBool("use distant fog", "Fog"))
        , mUnderwaterColor(Fallback::Map::getColour("Water_UnderwaterColor"))
        , mUnderwaterWeight(Fallback::Map::getFloat("Water_UnderwaterColorWeight"))
        , mUnderwaterIndoorFog(Fallback::Map::getFloat("Water_UnderwaterIndoorFog"))
    {
        DLLandFogStart = Settings::Manager::getFloat("distant land fog start", "Fog");
        DLLandFogEnd = Settings::Manager::getFloat("distant land fog end", "Fog");
        DLUnderwaterFogStart = Settings::Manager::getFloat("distant underwater fog start", "Fog");
        DLUnderwaterFogEnd = Settings::Manager::getFloat("distant underwater fog end", "Fog");
        DLInteriorFogStart = Settings::Manager::getFloat("distant interior fog start", "Fog");
        DLInteriorFogEnd = Settings::Manager::getFloat("distant interior fog end", "Fog");
    }

    void FogManager::configure(float viewDistance, const ESM::CellVariant& cell)
    {
        auto cell3 = cell.getEsm3();
        auto cell4 = cell.getEsm4();

        osg::Vec4f color
            = SceneUtil::colourFromRGB(cell.isEsm4() ? cell.getEsm4().mLighting.fogColor : cell.getEsm3().mAmbi.mFog);

        const float fogDensity = !cell.isEsm4() ? cell.getEsm3().mAmbi.mFogDensity : cell.getEsm4().mLighting.fogPower;
        if (mDistantFog)
        {
            float density = std::max(0.2f, fogDensity);
            mLandFogStart = DLInteriorFogEnd * (1.0f - density) + DLInteriorFogStart * density;
            mLandFogEnd = DLInteriorFogEnd;
            mUnderwaterFogStart = DLUnderwaterFogStart;
            mUnderwaterFogEnd = DLUnderwaterFogEnd;
            mFogColor = color;
        }
        else
            configure(viewDistance, fogDensity, mUnderwaterIndoorFog, 1.0f, 0.0f, color);
    }

    void FogManager::configure(float viewDistance, float fogDepth, float underwaterFog, float dlFactor, float dlOffset,
        const osg::Vec4f& color)
    {
        if (mDistantFog)
        {
            mLandFogStart = dlFactor * (DLLandFogStart - dlOffset * DLLandFogEnd);
            mLandFogEnd = dlFactor * (1.0f - dlOffset) * DLLandFogEnd;
            mUnderwaterFogStart = DLUnderwaterFogStart;
            mUnderwaterFogEnd = DLUnderwaterFogEnd;
        }
        else
        {
            if (fogDepth == 0.0)
            {
                mLandFogStart = 0.0f;
                mLandFogEnd = std::numeric_limits<float>::max();
            }
            else
            {
                mLandFogStart = viewDistance * (1 - fogDepth);
                mLandFogEnd = viewDistance;
            }
            mUnderwaterFogStart = std::min(viewDistance, 7168.f) * (1 - underwaterFog);
            mUnderwaterFogEnd = std::min(viewDistance, 7168.f);
        }
        mFogColor = color;
    }

    float FogManager::getFogStart(bool isUnderwater) const
    {
        return isUnderwater ? mUnderwaterFogStart : mLandFogStart;
    }

    float FogManager::getFogEnd(bool isUnderwater) const
    {
        return isUnderwater ? mUnderwaterFogEnd : mLandFogEnd;
    }

    osg::Vec4f FogManager::getFogColor(bool isUnderwater) const
    {
        if (isUnderwater)
        {
            return mUnderwaterColor * mUnderwaterWeight + mFogColor * (1.f - mUnderwaterWeight);
        }

        return mFogColor;
    }
}
