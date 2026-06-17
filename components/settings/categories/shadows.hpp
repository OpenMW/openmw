#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADOWS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADOWS_H

#include <components/settings/sanitizerimpl.hpp>
#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct ShadowsCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mEnableShadows{ mIndex, "Shadows", "enable shadows" };
        SettingValue<int> mNumberOfShadowMaps{ mIndex, "Shadows", "number of shadow maps",
            makeClampSanitizerInt(1, 8) };
        SettingValue<float> mMaximumShadowMapDistance{ mIndex, "Shadows", "maximum shadow map distance" };
        SettingValue<float> mShadowFadeStart{ mIndex, "Shadows", "shadow fade start", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mSplitPointUniformLogarithmicRatio{ mIndex, "Shadows",
            "split point uniform logarithmic ratio", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mSplitPointBias{ mIndex, "Shadows", "split point bias" };
        SettingValue<bool> mEnableDebugHud{ mIndex, "Shadows", "enable debug hud" };
        SettingValue<bool> mEnableDebugOverlay{ mIndex, "Shadows", "enable debug overlay" };
        SettingValue<std::string> mComputeSceneBounds{ mIndex, "Shadows", "compute scene bounds",
            makeEnumSanitizerString({ "primitives", "bounds", "none" }) };
        SettingValue<int> mShadowMapResolution{ mIndex, "Shadows", "shadow map resolution" };
        SettingValue<float> mMinimumLispsmNearFarRatio{ mIndex, "Shadows", "minimum lispsm near far ratio",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mPolygonOffsetFactor{ mIndex, "Shadows", "polygon offset factor" };
        SettingValue<float> mPolygonOffsetUnits{ mIndex, "Shadows", "polygon offset units" };
        SettingValue<float> mNormalOffsetDistance{ mIndex, "Shadows", "normal offset distance" };
        SettingValue<bool> mUseFrontFaceCulling{ mIndex, "Shadows", "use front face culling" };
        SettingValue<bool> mActorShadows{ mIndex, "Shadows", "actor shadows" };
        SettingValue<bool> mPlayerShadows{ mIndex, "Shadows", "player shadows" };
        SettingValue<bool> mTerrainShadows{ mIndex, "Shadows", "terrain shadows" };
        SettingValue<bool> mObjectShadows{ mIndex, "Shadows", "object shadows" };
        SettingValue<bool> mEnableIndoorShadows{ mIndex, "Shadows", "enable indoor shadows" };
    };
}

#endif
