
#include "category.hpp"

#include <stdexcept>

#include "setting.hpp"
#include "state.hpp"
#include "subcategory.hpp"

CSMPrefs::Category::Category(State* parent, const std::string& key)
    : mParent(parent)
    , mKey(key)
{
}

const std::string& CSMPrefs::Category::getKey() const
{
    return mKey;
}

CSMPrefs::State* CSMPrefs::Category::getState() const
{
    return mParent;
}

void CSMPrefs::Category::addSetting(Setting* setting)
{
    if (!mIndex.emplace(setting->getKey(), setting).second)
        throw std::logic_error("Category " + mKey + " already has setting: " + setting->getKey());

    mSettings.push_back(setting);
}

void CSMPrefs::Category::addSubcategory(Subcategory* setting)
{
    mSettings.push_back(setting);
}

CSMPrefs::Category::Iterator CSMPrefs::Category::begin()
{
    return mSettings.begin();
}

CSMPrefs::Category::Iterator CSMPrefs::Category::end()
{
    return mSettings.end();
}

CSMPrefs::Setting& CSMPrefs::Category::operator[](const std::string& key)
{
    const auto it = mIndex.find(key);

    if (it != mIndex.end())
        return *it->second;

    throw std::logic_error("Invalid user setting in " + mKey + " category: " + key);
}

void CSMPrefs::Category::update()
{
    for (Iterator iter = mSettings.begin(); iter != mSettings.end(); ++iter)
        mParent->update(**iter);
}
