#include "settings.hpp"

#include <fstream>

#include <OgreResourceGroupManager.h>
#include <OgreStringConverter.h>

using namespace Settings;

Ogre::ConfigFile Manager::mFile = Ogre::ConfigFile();
Ogre::ConfigFile Manager::mDefaultFile = Ogre::ConfigFile();

void Manager::load(const std::string& file)
{
    mFile.load(file);
}

void Manager::loadDefault(const std::string& file)
{
    mDefaultFile.load(file);
}

void Manager::save(const std::string& file)
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
