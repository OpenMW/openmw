
#include "category.hpp"

#include <stdexcept>

#include "setting.hpp"
#include "state.hpp"

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

CSMPrefs::Setting& CSMPrefs::Category::operator[] (const std::string& key)
{
    for (Iterator iter = mSettings.begin(); iter!=mSettings.end(); ++iter)
        if ((*iter)->getKey()==key)
            return **iter;

    throw std::logic_error ("Invalid user setting: " + key);
}

void CSMPrefs::Category::update()
{
    for (Iterator iter = mSettings.begin(); iter!=mSettings.end(); ++iter)
        mParent->update (**iter);
}
