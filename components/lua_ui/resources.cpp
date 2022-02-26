#include "resources.hpp"

#include <components/vfs/manager.hpp>
#include <components/misc/stringops.hpp>

namespace LuaUi
{
    std::shared_ptr<TextureResource> ResourceManager::registerTexture(TextureData data)
    {
        std::string normalizedPath = mVfs->normalizeFilename(data.mPath);
        if (!mVfs->exists(normalizedPath))
        {
            auto error = Misc::StringUtils::format("Texture with path \"%s\" doesn't exist", data.mPath);
            throw std::logic_error(error);
        }
        data.mPath = normalizedPath;
        
        TextureResources& list = mTextures[normalizedPath];
        list.push_back(std::make_shared<TextureResource>(data));
        return list.back();
    }

    void ResourceManager::clear()
    {
        mTextures.clear();
    }
}
