
#include "resourcesmanager.hpp"

#include <stdexcept>

void CSMWorld::ResourcesManager::addResources (const Resources& resources)
{
    mResources.insert (std::make_pair (resources.getType(), resources));
    mResources.insert (std::make_pair (UniversalId::getParentType (resources.getType()),
        resources));
}

void CSMWorld::ResourcesManager::setVFS(const VFS::Manager *vfs)
{
    mResources.clear();

    static const char * const sMeshTypes[] = { "nif", 0 };

    addResources (Resources (vfs, "meshes", UniversalId::Type_Mesh, sMeshTypes));
    addResources (Resources (vfs, "icons", UniversalId::Type_Icon));
    addResources (Resources (vfs, "music", UniversalId::Type_Music));
    addResources (Resources (vfs, "sound", UniversalId::Type_SoundRes));
    addResources (Resources (vfs, "textures", UniversalId::Type_Texture));
    addResources (Resources (vfs, "videos", UniversalId::Type_Video));
}

const CSMWorld::Resources& CSMWorld::ResourcesManager::get (UniversalId::Type type) const
{
    std::map<UniversalId::Type, Resources>::const_iterator iter = mResources.find (type);

    if (iter==mResources.end())
        throw std::logic_error ("Unknown resource type");

    return iter->second;
}
