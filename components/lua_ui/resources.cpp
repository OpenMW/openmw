#include "resources.hpp"

#include <components/vfs/pathutil.hpp>

namespace LuaUi
{
    std::shared_ptr<TextureResource> ResourceManager::registerTexture(TextureData data)
    {
        TextureResources& list = mTextures[data.mPath];
        list.push_back(std::make_shared<TextureResource>(data));
        return list.back();
    }

    void ResourceManager::clear()
    {
        mTextures.clear();
    }
}
