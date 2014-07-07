
#include "resourcesmanager.hpp"

#include <stdexcept>

void CSMWorld::ResourcesManager::listResources()
{
    static const char * const sMeshTypes[] = { "nif", 0 };

    mResources.insert (
        std::make_pair (UniversalId::Type_Mesh, Resources ("meshes", sMeshTypes)));
    mResources.insert (std::make_pair (UniversalId::Type_Icon, "icons"));
    mResources.insert (std::make_pair (UniversalId::Type_Music, "music"));
    mResources.insert (std::make_pair (UniversalId::Type_SoundRes, "sound"));
    mResources.insert (std::make_pair (UniversalId::Type_Texture, "textures"));
    mResources.insert (std::make_pair (UniversalId::Type_Video, "videos"));
}

const CSMWorld::Resources& CSMWorld::ResourcesManager::get (UniversalId::Type type) const
{
    std::map<UniversalId::Type, Resources>::const_iterator iter = mResources.find (type);

    if (iter==mResources.end())
        throw std::logic_error ("Unknown resource type");

    return iter->second;
}