
#include "setting.hpp"

#include "category.hpp"
#include "state.hpp"

Settings::Manager& CSMPrefs::Setting::getValues()
{
    return *mValues;
}

CSMPrefs::Setting::Setting (Category *parent, Settings::Manager *values,
    const std::string& key, const std::string& label)
: QObject (parent->getState()), mParent (parent), mValues (values), mKey (key), mLabel (label)
{}

CSMPrefs::Setting:: ~Setting() {}

std::pair<QWidget *, QWidget *> CSMPrefs::Setting::makeWidgets (QWidget *parent)
{
    return std::pair<QWidget *, QWidget *> (0, 0);
}

const CSMPrefs::Category *CSMPrefs::Setting::getParent() const
{
    return mParent;
}

const std::string& CSMPrefs::Setting::getKey() const
{
    return mKey;
}

const std::string& CSMPrefs::Setting::getLabel() const
{
    return mLabel;
}
