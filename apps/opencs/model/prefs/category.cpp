
#include "category.hpp"

CSMPrefs::Category::Category (State *parent, const std::string& key, const std::string& name)
: mParent (parent), mKey (key), mName (name)
{}

const std::string& CSMPrefs::Category::getKey() const
{
    return mKey;
}

const std::string& CSMPrefs::Category::getName() const
{
    return mName;
}
