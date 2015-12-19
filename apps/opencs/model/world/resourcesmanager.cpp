#include "resourcesmanager.hpp"

#include <stdexcept>

#include <OgreResourceGroupManager.h>

void CSMWorld::ResourcesManager::addResources (const Resources& resources)
{
    mResources.insert (std::make_pair (resources.getType(), resources));
    mResources.insert (std::make_pair (UniversalId::getParentType (resources.getType()),
        resources));
}

void CSMWorld::ResourcesManager::listResources()
{
    // Following code was taken out of Resources ctor, since it was being executed each time
    // and slow enough to showe up in the profiler.
    //
    // See Editor ctor which calls Bsa::registerResources()
    //
    // resourceGroups include those from config files, e.g.:
    //
    //  C:/Program Files\OpenMW\data
    //  C:/Program Files (x86)\Bethesda Softworks\Morrowind\Data Files
    //
    // and from archives:
    //
    //  C:/Program Files (x86)\Bethesda Softworks\Morrowind\Data Files\Morrowind.bsa
    //  C:/Program Files (x86)\Bethesda Softworks\Morrowind\Data Files\Tribunal.bsa
    //  C:/Program Files (x86)\Bethesda Softworks\Morrowind\Data Files\Bloodmoon.bsa
    //
    std::vector<Ogre::StringVectorPtr> resources;

    Ogre::StringVector resourcesGroups =
        Ogre::ResourceGroupManager::getSingleton().getResourceGroups();

    for (Ogre::StringVector::iterator iter (resourcesGroups.begin());
        iter!=resourcesGroups.end(); ++iter)
    {
        if (*iter=="General" || *iter=="Internal" || *iter=="Autodetect")
            continue;

        resources.push_back(
            Ogre::ResourceGroupManager::getSingleton().listResourceNames (*iter));
    }

    static const char * const sMeshTypes[] = { "nif", 0 };

    addResources (Resources ("meshes", UniversalId::Type_Mesh, resources, sMeshTypes));
    addResources (Resources ("icons", UniversalId::Type_Icon, resources));
    addResources (Resources ("music", UniversalId::Type_Music, resources));
    addResources (Resources ("sound", UniversalId::Type_SoundRes, resources));
    addResources (Resources ("textures", UniversalId::Type_Texture, resources));
    addResources (Resources ("videos", UniversalId::Type_Video, resources));
}

const CSMWorld::Resources& CSMWorld::ResourcesManager::get (UniversalId::Type type) const
{
    std::map<UniversalId::Type, Resources>::const_iterator iter = mResources.find (type);

    if (iter==mResources.end())
        throw std::logic_error ("Unknown resource type");

    return iter->second;
}
