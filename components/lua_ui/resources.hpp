#ifndef OPENMW_LUAUI_RESOURCES
#define OPENMW_LUAUI_RESOURCES

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include <osg/Vec2f>

namespace VFS
{
    class Manager;
}

namespace LuaUi
{
    struct TextureData
    {
        std::string mPath;
        osg::Vec2f mOffset;
        osg::Vec2f mSize;
    };

    // will have more/different data when automated atlasing is supported
    using TextureResource = TextureData;

    class ResourceManager
    {
        public:
            ResourceManager(const VFS::Manager* vfs)
                : mVfs(vfs)
            {}

            std::shared_ptr<TextureResource> registerTexture(TextureData data);
            void clear();

        private:
            const VFS::Manager* mVfs;
            using TextureResources = std::vector<std::shared_ptr<TextureResource>>;
            std::unordered_map<std::string, TextureResources> mTextures;
    };
}

#endif // OPENMW_LUAUI_LAYERS
