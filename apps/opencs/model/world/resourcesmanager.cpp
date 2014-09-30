
#include "resourcesmanager.hpp"

#include <stdexcept>

void CSMWorld::ResourcesManager::addResources (const Resources& resources)
{
    mResources.insert (std::make_pair (resources.getType(), resources));
}

void CSMWorld::ResourcesManager::listResources()
{
    static const char * const sMeshTypes[] = { "nif", 0 };

    addResources (Resources ("meshes", UniversalId::Type_Meshes, sMeshTypes));
    addResources (Resources ("icons", UniversalId::Type_Icons));
    addResources (Resources ("music", UniversalId::Type_Musics));
    addResources (Resources ("sound", UniversalId::Type_SoundsRes));
    addResources (Resources ("textures", UniversalId::Type_Textures));
    addResources (Resources ("videos", UniversalId::Type_Videos));
}

const CSMWorld::Resources& CSMWorld::ResourcesManager::get (UniversalId::Type type) const
{
    std::map<UniversalId::Type, Resources>::const_iterator iter = mResources.find (type);

    if (iter==mResources.end())
        throw std::logic_error ("Unknown resource type");

    return iter->second;
}