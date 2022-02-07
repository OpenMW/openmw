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

    class TextureResource
    {
        public:
            TextureResource(TextureData data)
                : mData(data)
                {}

            const TextureData& data() { return mData; }
        private:
            TextureData mData;
    };

    class ResourceManager {
        public:
            ResourceManager(const VFS::Manager* vfs)
                : mVfs(vfs)
            {}

            std::shared_ptr<TextureResource> registerTexture(TextureData data);

        protected:
            const VFS::Manager* vfs() const { return mVfs; }

        private:
            const VFS::Manager* mVfs;
            using TextureResources = std::vector<std::shared_ptr<TextureResource>>;
            std::unordered_map<std::string, TextureResources> mTextures;
    };
}

#endif // OPENMW_LUAUI_LAYERS
