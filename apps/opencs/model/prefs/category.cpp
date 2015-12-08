
#include "category.hpp"

CSMPrefs::Category::Category (State *parent, const std::string& key)
: mParent (parent), mKey (key)
{}

const std::string& CSMPrefs::Category::getKey() const
{
    return mKey;
}
