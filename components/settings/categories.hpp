#ifndef COMPONENTS_SETTINGS_CATEGORIES_H
#define COMPONENTS_SETTINGS_CATEGORIES_H

#include <map>
#include <set>
#include <string>
#include <utility>

namespace Settings
{
    using CategorySetting = std::pair<std::string, std::string>;
    using CategorySettingVector = std::set<std::pair<std::string, std::string>>;
    using CategorySettingValueMap = std::map<CategorySetting, std::string>;
}

#endif // COMPONENTS_SETTINGS_CATEGORIES_H
