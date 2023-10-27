#ifndef OPENMW_COMPONENTS_SETTINGS_SCREENSHOTSETTINGS_H
#define OPENMW_COMPONENTS_SETTINGS_SCREENSHOTSETTINGS_H

#include <optional>
#include <ostream>

namespace Settings
{
    enum class ScreenshotType
    {
        Regular,
        Cylindrical,
        Spherical,
        Planet,
        Cubemap,
    };

    struct ScreenshotSettings
    {
        ScreenshotType mType;
        std::optional<int> mWidth;
        std::optional<int> mHeight;
        std::optional<int> mCubeSize;

        auto operator<=>(const ScreenshotSettings& value) const = default;
    };
}

#endif
