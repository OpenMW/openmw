#ifndef COMPONENTS_SETTINGS_CATEGORIES_H
#define COMPONENTS_SETTINGS_CATEGORIES_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <string_view>

namespace Settings
{
    struct Less
    {
        using is_transparent = void;

        bool operator()(const std::pair<std::string_view, std::string_view>& l,
            const std::pair<std::string_view, std::string_view>& r) const
        {
            return l < r;
        }
    };

    using CategorySetting = std::pair<std::string, std::string>;
    using CategorySettingVector = std::set<CategorySetting>;
    using CategorySettingValueMap = std::map<CategorySetting, std::string, Less>;
}

#endif // COMPONENTS_SETTINGS_CATEGORIES_H
