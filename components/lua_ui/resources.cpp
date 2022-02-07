#include "resources.hpp"

#include <components/vfs/manager.hpp>

namespace LuaUi
{
    std::shared_ptr<TextureResource> ResourceManager::registerTexture(TextureData data)
    {
        std::string normalizedPath = vfs()->normalizeFilename(data.mPath);
        if (!vfs()->exists(normalizedPath))
        {
            std::string error("Texture with path \"");
            error += data.mPath;
            error += "\" doesn't exist";
            throw std::logic_error(error);
        }
        data.mPath = normalizedPath;
        
        TextureResources& list = mTextures[normalizedPath];
        list.push_back(std::make_shared<TextureResource>(data));
        return list.back();
    }
}
