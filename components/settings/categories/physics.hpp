#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_PHYSICS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_PHYSICS_H

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
    struct PhysicsCategory
    {
        SettingValue<int> mAsyncNumThreads{ "Physics", "async num threads", makeMaxSanitizerInt(0) };
        SettingValue<int> mLineofsightKeepInactiveCache{ "Physics", "lineofsight keep inactive cache",
            makeMaxSanitizerInt(-1) };
    };
}

#endif
