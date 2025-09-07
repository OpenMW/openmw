#ifndef OPENMW_LUAUI_RESOURCES
#define OPENMW_LUAUI_RESOURCES

#include <memory>
#include <unordered_map>
#include <vector>

#include <osg/Vec2f>

#include <components/vfs/pathutil.hpp>

namespace VFS
{
    class Manager;
}

namespace LuaUi
{
    struct TextureData
    {
        VFS::Path::Normalized mPath;
        osg::Vec2f mOffset;
        osg::Vec2f mSize;
    };

    // will have more/different data when automated atlasing is supported
    using TextureResource = TextureData;

    class ResourceManager
    {
    public:
        std::shared_ptr<TextureResource> registerTexture(TextureData data);
        void clear();

    private:
        using TextureResources = std::vector<std::shared_ptr<TextureResource>>;
        std::unordered_map<VFS::Path::Normalized, TextureResources, VFS::Path::Hash> mTextures;
    };
}

#endif // OPENMW_LUAUI_LAYERS
