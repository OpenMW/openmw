#include "resources.hpp"

#include <components/vfs/manager.hpp>
#include <components/misc/stringops.hpp>

namespace LuaUi
{
    std::shared_ptr<TextureResource> ResourceManager::registerTexture(TextureData data)
    {
        data.mPath = mVfs->normalizeFilename(data.mPath);

        TextureResources& list = mTextures[data.mPath];
        list.push_back(std::make_shared<TextureResource>(data));
        return list.back();
    }

    void ResourceManager::clear()
    {
        mTextures.clear();
    }
}
