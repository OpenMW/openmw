#include "settings.hpp"

#include <fstream>

#include <OgreResourceGroupManager.h>
#include <OgreStringConverter.h>

using namespace Settings;

Ogre::ConfigFile Manager::mFile = Ogre::ConfigFile();
Ogre::ConfigFile Manager::mDefaultFile = Ogre::ConfigFile();
SettingCategoryVector Manager::mChangedSettings = SettingCategoryVector();

void Manager::loadUser (const std::string& file)
{
    mFile.load(file);
}

void Manager::loadDefault (const std::string& file)
{
    mDefaultFile.load(file);
}

void Manager::copyDefaultToUserSettings ()
{
    mFile = mDefaultFile;
}

void Manager::saveUser(const std::string& file)
{
    std::fstream fout(file.c_str(), std::ios::out);

    Ogre::ConfigFile::SectionIterator seci = mFile.getSectionIterator();

    while (seci.hasMoreElements())
    {
        Ogre::String sectionName = seci.peekNextKey();

        if (sectionName.length() > 0)
            fout << '\n' << '[' << seci.peekNextKey() << ']' << '\n';

        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            fout << i->first.c_str() << '=' << i->second.c_str() << '\n';
        }

        seci.getNext();
    }
}

const std::string Manager::getString (const std::string& setting, const std::string& category)
{
    std::string defaultval = mDefaultFile.getSetting(setting, category);
    return mFile.getSetting(setting, category, defaultval);
}

const float Manager::getFloat (const std::string& setting, const std::string& category)
{
    return Ogre::StringConverter::parseReal( getString(setting, category) );
}

const int Manager::getInt (const std::string& setting, const std::string& category)
{
    return Ogre::StringConverter::parseInt( getString(setting, category) );
}

const bool Manager::getBool (const std::string& setting, const std::string& category)
{
    return Ogre::StringConverter::parseBool( getString(setting, category) );
}

void Manager::setString (const std::string& setting, const std::string& category, const std::string& value)
{
    Ogre::ConfigFile::SettingsIterator it = mFile.getSettingsIterator(category);
    while (it.hasMoreElements())
    {
        Ogre::ConfigFile::SettingsMultiMap::iterator i = it.current();

        if ((*i).first == setting && (*i).second != value)
        {
            mChangedSettings.push_back(std::make_pair(setting, category));
            (*i).second = value;
        }

        it.getNext();
    }
}

void Manager::setInt (const std::string& setting, const std::string& category, const int value)
{
    setString(setting, category, Ogre::StringConverter::toString(value));
}

void Manager::setFloat (const std::string& setting, const std::string& category, const float value)
{
    setString(setting, category, Ogre::StringConverter::toString(value));
}

void Manager::setBool (const std::string& setting, const std::string& category, const bool value)
{
    setString(setting, category, Ogre::StringConverter::toString(value));
}

const SettingCategoryVector Manager::apply()
{
    SettingCategoryVector vec = mChangedSettings;
    mChangedSettings.clear();
    return vec;
}
