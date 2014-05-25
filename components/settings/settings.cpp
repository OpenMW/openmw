#include "settings.hpp"

#include <stdexcept>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreStringConverter.h>
#include <OgreDataStream.h>

#include <components/files/constrainedfiledatastream.hpp>

using namespace Settings;

namespace bfs = boost::filesystem;

Ogre::ConfigFile Manager::mFile = Ogre::ConfigFile();
Ogre::ConfigFile Manager::mDefaultFile = Ogre::ConfigFile();
CategorySettingVector Manager::mChangedSettings = CategorySettingVector();
CategorySettingValueMap Manager::mNewSettings = CategorySettingValueMap();

void Manager::loadUser (const std::string& file)
{
    Ogre::DataStreamPtr stream = openConstrainedFileDataStream(file.c_str());
    mFile.load(stream);
}

void Manager::loadDefault (const std::string& file)
{
    Ogre::DataStreamPtr stream = openConstrainedFileDataStream(file.c_str());
    mDefaultFile.load(stream);
}

void Manager::saveUser(const std::string& file)
{
    bfs::ofstream fout((bfs::path(file)));

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
            fout << i->first.c_str() << " = " << i->second.c_str() << '\n';
        }

        CategorySettingValueMap::iterator it = mNewSettings.begin();
        while (it != mNewSettings.end())
        {
            if (it->first.first == sectionName)
            {
                fout << it->first.second << " = " << it->second << '\n';
                mNewSettings.erase(it++);
            }
            else
                ++it;
        }
    }

    std::string category = "";
    for (CategorySettingValueMap::iterator it = mNewSettings.begin();
            it != mNewSettings.end(); ++it)
    {
        if (category != it->first.first)
        {
            category = it->first.first;
            fout << '\n' << '[' << category << ']' << '\n';
        }
        fout << it->first.second << " = " << it->second << '\n';
    }

    fout.close();
}

const std::string Manager::getString (const std::string& setting, const std::string& category)
{
    if (mNewSettings.find(std::make_pair(category, setting)) != mNewSettings.end())
        return mNewSettings[std::make_pair(category, setting)];

    std::string defaultval = mDefaultFile.getSetting(setting, category, "NOTFOUND");
    std::string val = mFile.getSetting(setting, category, defaultval);

    if (val == "NOTFOUND")
        throw std::runtime_error("Trying to retrieve a non-existing setting: " + setting + " Make sure the settings-default.cfg file was properly installed.");
    return val;
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
    CategorySetting s = std::make_pair(category, setting);

    bool found=false;
    try
    {
        Ogre::ConfigFile::SettingsIterator it = mFile.getSettingsIterator(category);
        while (it.hasMoreElements())
        {
            Ogre::ConfigFile::SettingsMultiMap::iterator i = it.current();

            if ((*i).first == setting)
            {
                if ((*i).second != value)
                {
                    mChangedSettings.push_back(std::make_pair(category, setting));
                    (*i).second = value;
                }
                found = true;
            }

            it.getNext();
        }
    }
    catch (Ogre::Exception&)
    {}

    if (!found)
    {
        if (mNewSettings.find(s) != mNewSettings.end())
        {
            if (mNewSettings[s] != value)
            {
                mChangedSettings.push_back(std::make_pair(category, setting));
                mNewSettings[s] = value;
            }
        }
        else
        {
            if (mDefaultFile.getSetting(setting, category) != value)
                mChangedSettings.push_back(std::make_pair(category, setting));
            mNewSettings[s] = value;
        }
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

const CategorySettingVector Manager::apply()
{
    CategorySettingVector vec = mChangedSettings;
    mChangedSettings.clear();
    return vec;
}
