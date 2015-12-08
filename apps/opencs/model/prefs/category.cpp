
#include "category.hpp"

CSMPrefs::Category::Category (State *parent, const std::string& key)
: mParent (parent), mKey (key)
{}

const std::string& CSMPrefs::Category::getKey() const
{
    return mKey;
}

CSMPrefs::State *CSMPrefs::Category::getState() const
{
    return mParent;
}

void CSMPrefs::Category::addSetting (Setting *setting)
{
    mSettings.push_back (setting);
}

CSMPrefs::Category::Iterator CSMPrefs::Category::begin()
{
    return mSettings.begin();
}

CSMPrefs::Category::Iterator CSMPrefs::Category::end()
{
    return mSettings.end();
}
