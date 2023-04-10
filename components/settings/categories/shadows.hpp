#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADOWS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADOWS_H

#include "components/settings/sanitizerimpl.hpp"
#include "components/settings/settingvalue.hpp"

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct ShadowsCategory
    {
        SettingValue<bool> mEnableShadows{ "Shadows", "enable shadows" };
        SettingValue<int> mNumberOfShadowMaps{ "Shadows", "number of shadow maps", makeClampSanitizerInt(1, 8) };
        SettingValue<float> mMaximumShadowMapDistance{ "Shadows", "maximum shadow map distance" };
        SettingValue<float> mShadowFadeStart{ "Shadows", "shadow fade start", makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mAllowShadowMapOverlap{ "Shadows", "allow shadow map overlap" };
        SettingValue<float> mSplitPointUniformLogarithmicRatio{ "Shadows", "split point uniform logarithmic ratio",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mSplitPointBias{ "Shadows", "split point bias" };
        SettingValue<bool> mEnableDebugHud{ "Shadows", "enable debug hud" };
        SettingValue<bool> mEnableDebugOverlay{ "Shadows", "enable debug overlay" };
        SettingValue<std::string> mComputeSceneBounds{ "Shadows", "compute scene bounds",
            makeEnumSanitizerString({ "primitives", "bounds", "none" }) };
        SettingValue<int> mShadowMapResolution{ "Shadows", "shadow map resolution" };
        SettingValue<float> mMinimumLispsmNearFarRatio{ "Shadows", "minimum lispsm near far ratio",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mPolygonOffsetFactor{ "Shadows", "polygon offset factor" };
        SettingValue<float> mPolygonOffsetUnits{ "Shadows", "polygon offset units" };
        SettingValue<float> mNormalOffsetDistance{ "Shadows", "normal offset distance" };
        SettingValue<bool> mUseFrontFaceCulling{ "Shadows", "use front face culling" };
        SettingValue<bool> mActorShadows{ "Shadows", "actor shadows" };
        SettingValue<bool> mPlayerShadows{ "Shadows", "player shadows" };
        SettingValue<bool> mTerrainShadows{ "Shadows", "terrain shadows" };
        SettingValue<bool> mObjectShadows{ "Shadows", "object shadows" };
        SettingValue<bool> mEnableIndoorShadows{ "Shadows", "enable indoor shadows" };
    };
}

#endif
