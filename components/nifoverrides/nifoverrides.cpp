#include "nifoverrides.hpp"

#include <OgreStringConverter.h>

#include <../components/misc/stringops.hpp>

#include "../extern/shiny/Main/MaterialInstance.hpp"

#include <stdexcept>


using namespace NifOverrides;

Overrides::TransparencyOverrideMap Overrides::mTransparencyOverrides = Overrides::TransparencyOverrideMap();
Overrides::MaterialOverrideMap Overrides::mMaterialOverrides = Overrides::MaterialOverrideMap();

void Overrides::loadTransparencyOverrides (const std::string& file)
{
    Ogre::ConfigFile cf;
    cf.load(file);

    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
    while (seci.hasMoreElements())
    {
        Ogre::String sectionName = seci.peekNextKey();
        mTransparencyOverrides[sectionName] =
                Ogre::StringConverter::parseInt(cf.getSetting("alphaRejectValue", sectionName));
        seci.getNext();
    }
}

void Overrides::loadMaterialOverrides(const std::string &file)
{
    Ogre::ConfigFile cf;
    cf.load(file);

    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
    while (seci.hasMoreElements())
    {
        Ogre::String sectionName = seci.peekNextKey();

        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        std::map<std::string, std::string> overrides;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            overrides[i->first] = i->second;
        }
        mMaterialOverrides[sectionName] = overrides;
    }

}

TransparencyResult Overrides::getTransparencyOverride(const std::string& texture)
{
    TransparencyResult result;
    result.first = false;

    TransparencyOverrideMap::iterator it = mTransparencyOverrides.find(Misc::StringUtils::lowerCase(texture));
    if (it != mTransparencyOverrides.end())
    {
        result.first = true;
        result.second = it->second;
    }

    return result;
}

void Overrides::getMaterialOverrides(const std::string &texture, sh::MaterialInstance* material)
{
    MaterialOverrideMap::iterator it = mMaterialOverrides.find(Misc::StringUtils::lowerCase(texture));
    if (it != mMaterialOverrides.end())
    {
        const std::map<std::string, std::string>& overrides = it->second;
        for (std::map<std::string, std::string>::const_iterator it = overrides.begin(); it != overrides.end(); ++it)
        {
            material->setProperty(it->first, sh::makeProperty(it->second));
        }
    }
}
